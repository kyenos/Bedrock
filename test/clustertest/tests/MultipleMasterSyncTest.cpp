#include "../BedrockClusterTester.h"

struct MultipleMasterSyncTest : tpunit::TestFixture {
    MultipleMasterSyncTest()
        : tpunit::TestFixture("MultipleMasterSyncTest",
                              BEFORE_CLASS(MultipleMasterSyncTest::setup),
                              AFTER_CLASS(MultipleMasterSyncTest::teardown),
                              TEST(MultipleMasterSyncTest::test)
                             ) { }

    BedrockClusterTester* tester;

    void setup() {
        // create a 5 node cluster
        tester = new BedrockClusterTester(BedrockClusterTester::FIVE_NODE_CLUSTER, {"CREATE TABLE test (id INTEGER NOT NULL PRIMARY KEY, value TEXT NOT NULL)"}, _threadID);

        // make sure the whole cluster is up
        ASSERT_TRUE(tester->getBedrockTester(0)->waitForState("MASTERING"));
        for (int i = 1; i <= 4; i++) {
            ASSERT_TRUE(tester->getBedrockTester(i)->waitForState("SLAVING"));
        }

        // shut down primary master
        tester->stopNode(0);

        // Wait for node 1 to be master.
        ASSERT_TRUE(tester->getBedrockTester(1)->waitForState("MASTERING"));

        // increase the delta between primary and secondary master
        runTrivialWrites(5000, 4);

        // shut down secondary master
        tester->stopNode(1);

        // Wait for node 2 to be master.
        ASSERT_TRUE(tester->getBedrockTester(2)->waitForState("MASTERING"));

        // give secondary master a few commits to sync
        runTrivialWrites(5000, 4);

        // FYI: Because of the way getCommitCount() works, This only works once.
        // Check for the correct number of commits
        sleep(10);
        ASSERT_TRUE(tester->getBedrockTester(4)->getCommitCount() >= 10000);

        // just a check for ready state
        ASSERT_TRUE(tester->getBedrockTester(2)->waitForState("MASTERING"));
        ASSERT_TRUE(tester->getBedrockTester(3)->waitForState("SLAVING"));
        ASSERT_TRUE(tester->getBedrockTester(4)->waitForState("SLAVING"));
    }

    void teardown() {
        delete tester;
    }

    void runTrivialWrites(int writeCount, int nodeID = 0) {
        // Create a bunch of trivial write commands.
        int count = 0;
        while (count <= writeCount) {
            SData request;
            request.methodLine = "Query";
            if (count == 0) {
                //request["query"] = "INSERT OR REPLACE INTO test (id, value) VALUES(12345, COALESCE(value, 0) + 1 );";
                request["query"] = "INSERT OR REPLACE INTO test (id, value) VALUES(12345, 1 );";
            } else {
                request["query"] = "UPDATE test SET value=value + 1 WHERE id=12345;";
            }
            request["connection"] = "forget";
            tester->getBedrockTester(nodeID)->executeWaitVerifyContent(request, "202");
            count++;
        }

    }

    void test() {
        // Bring masters back up in reverse order, should go quickly to SYNCHRONIZING
        tester->startNodeDontWait(1);
        tester->startNodeDontWait(0);
        ASSERT_TRUE(tester->getBedrockTester(1)->waitForState("SYNCHRONIZING", 10'000'000, true ));
        ASSERT_TRUE(tester->getBedrockTester(0)->waitForState("SYNCHRONIZING", 10'000'000, true ));

        // tertiary master should still be MASTERING for a while
        ASSERT_TRUE(tester->getBedrockTester(2)->waitForState("MASTERING", 5'000'000 ));

        // secondary master should catch up first and go MASTERING, wait up to 30s
        ASSERT_TRUE(tester->getBedrockTester(1)->waitForState("MASTERING", 30'000'000 ));

        // when primary master catches up it should go MASTERING, wait up to 30s
        ASSERT_TRUE(tester->getBedrockTester(0)->waitForState("MASTERING", 30'000'000 ));
    }

} __MultipleMasterSyncTest;
