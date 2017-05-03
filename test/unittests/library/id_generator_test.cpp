#include "gtest/gtest.h"
#define private public

#include "library/id_generator.h"

/*class BaseIdGeneratorTest : public ::testing::Test
{
protected:
    BaseIdGeneratorTest()
    {
    }
    void SetUp()
    {
    }

    void TearDown()
    {
    }
    typedef BaseIdGenerator _module_under_test;
};

TEST_F(BaseIdGeneratorTest, SampleTest)
{
    uint32_t id_1 = _module_under_test::generate_new();
    uint32_t id_2 = _module_under_test::generate_new();
    uint32_t id_3 = _module_under_test::generate_new();

    EXPECT_NE(id_1, id_2);
    EXPECT_EQ(id_2 + 1, id_3);
}*/


TEST(BaseIdGeneratorTest, generate_new_uid_test)
{
    uint32_t id_1 = BaseIdGenerator<uint32_t>::new_id();
    uint32_t id_2 = BaseIdGenerator<uint32_t>::new_id();
    uint32_t id_3 = BaseIdGenerator<uint32_t>::new_id();

    EXPECT_NE(id_1, id_2);
    EXPECT_EQ(id_2 + 1, id_3);

    ProcessorId p_id = ProcessorIdGenerator::new_id();

    EXPECT_NE(id_1, p_id);
}
