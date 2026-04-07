#include <gtest/gtest.h>
#include <optional>
#include <string>

// Тест 1: персонализация сообщения
// Тестируем напрямую через вспомогательную функцию
static std::string personalize(const std::string& tmpl,
                               const std::string& name) {
    std::string result = tmpl;
    const std::string placeholder = "{name}";
    auto pos = result.find(placeholder);
    if (pos != std::string::npos) {
        result.replace(pos, placeholder.size(), name);
    }
    return result;
}

TEST(PersonalizeTest, ReplacesNamePlaceholder) {
    std::string result = personalize("Привет, {name}! Скидка 20%", "Михаил");
    EXPECT_EQ(result, "Привет, Михаил! Скидка 20%");
}

TEST(PersonalizeTest, NoPlaceholderUnchanged) {
    std::string result = personalize("Скидка 20% на всё", "Михаил");
    EXPECT_EQ(result, "Скидка 20% на всё");
}

TEST(PersonalizeTest, EmptyName) {
    std::string result = personalize("Привет, {name}!", "");
    EXPECT_EQ(result, "Привет, !");
}

// Тест 2: логика статуса для resend
static bool canResend(const std::string& status) {
    return status == "failed";
}

TEST(ResendTest, AllowsResendForFailed) {
    EXPECT_TRUE(canResend("failed"));
}

TEST(ResendTest, DeniesResendForSent) {
    EXPECT_FALSE(canResend("sent"));
}

TEST(ResendTest, DeniesResendForCreated) {
    EXPECT_FALSE(canResend("created"));
}

TEST(ResendTest, DeniesResendForSkipped) {
    EXPECT_FALSE(canResend("skipped"));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}