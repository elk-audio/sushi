#include "gtest/gtest.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_KEYWORD_MACRO
#define private public
#define protected public
ELK_POP_WARNING

#include "library/id_generator.h"

TEST(BaseIdGeneratorTest, GenerateNewUid)
{
    ObjectId id_1 = BaseIdGenerator<ObjectId>::new_id();
    ObjectId id_2 = BaseIdGenerator<ObjectId>::new_id();
    ObjectId id_3 = BaseIdGenerator<ObjectId>::new_id();

    /* Verify that generated ids are unique and consecutive */
    EXPECT_NE(id_1, id_2);
    EXPECT_EQ(id_2 + 1, id_3);

    ObjectId p_id = ProcessorIdGenerator::new_id();

    EXPECT_NE(id_1, p_id);
}
