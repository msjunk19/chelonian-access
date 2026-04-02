#include <unity.h>
#include <macro_config.hpp>

void setUp() {
}

void tearDown() {
}

void test_macro_magic_number_valid() {
    Macro m;
    m.magic = MACRO_MAGIC;
    m.step_count = 1;
    m.steps[0].relay_mask = 0b0001;
    m.steps[0].duration = 1000;
    m.steps[0].gap = 0;
    strncpy(m.name, "Test", sizeof(m.name));

    TEST_ASSERT_EQUAL(MACRO_MAGIC, m.magic);
    TEST_ASSERT_EQUAL(1, m.step_count);
    TEST_ASSERT_EQUAL(0b0001, m.steps[0].relay_mask);
}

void test_macro_magic_number_invalid() {
    Macro m;
    m.magic = 0xDEADBEEF;
    m.step_count = 1;

    TEST_ASSERT_NOT_EQUAL(MACRO_MAGIC, m.magic);
}

void test_macro_magic_default_config() {
    MacroConfig config = DEFAULT_MACRO_CONFIG;

    for (uint8_t i = 0; i < config.macro_count; i++) {
        TEST_ASSERT_EQUAL_MESSAGE(MACRO_MAGIC, config.macros[i].magic, 
            "Default macro should have valid magic number");
    }
}

void test_macro_default_count() {
    TEST_ASSERT_EQUAL(5, DEFAULT_MACRO_CONFIG.macro_count);
}

void test_macro_unlock_defaults() {
    Macro& unlock = const_cast<Macro&>(DEFAULT_MACRO_CONFIG.macros[0]);
    
    TEST_ASSERT_EQUAL_STRING("Unlock", unlock.name);
    TEST_ASSERT_EQUAL(1, unlock.step_count);
    TEST_ASSERT_EQUAL(0b0001, unlock.steps[0].relay_mask);
    TEST_ASSERT_EQUAL(1000, unlock.steps[0].duration);
}
