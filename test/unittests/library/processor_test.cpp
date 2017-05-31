#include "gtest/gtest.h"

#define private public
#define protected public

#include "library/processor.h"

using namespace sushi;

/* Implement dummies of virtual methods to we can instantiate a test class */
class ProcessorTest : public Processor
{
    virtual ~ProcessorTest() {}
    virtual void process_audio(const ChunkSampleBuffer& /*in_buffer*/,
                               ChunkSampleBuffer& /*out_buffer*/) override {}
    virtual void process_event(Event /*event*/) override {}
};


class TestProcessor : public ::testing::Test
{
protected:
    TestProcessor()
    {
    }
    void SetUp()
    {
        _module_under_test = new ProcessorTest;
    }
    void TearDown()
    {
        delete(_module_under_test);
    }
    Processor* _module_under_test;
};

TEST_F(TestProcessor, TestBasicProperties)
{
    /* Set the common properties and verify the changes are applied */
    _module_under_test->set_name(std::string("Processor 1"));
    EXPECT_EQ(_module_under_test->name(), "Processor 1");

    _module_under_test->set_label("processor_1");
    EXPECT_EQ("processor_1", _module_under_test->label());

    _module_under_test->set_enabled(true);
    EXPECT_TRUE(_module_under_test->enabled());
}

TEST_F(TestProcessor, TestParameterHandling)
{
    /* Register a single parameter and verify accessor functions */
    auto p = new FloatParameterDescriptor("param", "Float", nullptr);
    _module_under_test->register_parameter(p);

    auto param = _module_under_test->parameter_from_name("not_found");
    EXPECT_FALSE(param);
    param = _module_under_test->parameter_from_name("param");
    EXPECT_TRUE(param);

    ObjectId id = param->id();
    param = _module_under_test->parameter_from_id(id);
    EXPECT_TRUE(param);
    param = _module_under_test->parameter_from_id(1000);
    EXPECT_FALSE(param);

    auto param_list = _module_under_test->all_parameters();
    EXPECT_EQ(1u, param_list.size());
}