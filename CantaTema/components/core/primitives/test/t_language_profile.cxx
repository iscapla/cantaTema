/**
 * @file t_language_profile.cxx
 * @brief Unit tests for ILanguageProfile, SpanishLanguageProfile, EnglishLanguageProfile, and LanguageProfileManager.
 */

#include <gtest/gtest.h>
#include "primitives/spanish_language_profile.hpp"
#include "primitives/english_language_profile.hpp"
#include "primitives/language_profile_manager.hpp"
#include "configuration/configuration_system.hpp"

TEST(TestLanguageProfile, SpanishProfileNormalization) {
    SpanishLanguageProfile profile;
    EXPECT_EQ(profile.get_language_code(), "es");

    EXPECT_EQ(profile.normalize_word("¿Extinción!"), "extincion");
    EXPECT_EQ(profile.normalize_word("¡Constitución!"), "constitucion");
    EXPECT_EQ(profile.normalize_word("Árbol"), "arbol");
    EXPECT_EQ(profile.normalize_word("Éxito"), "exito");
    EXPECT_EQ(profile.normalize_word("Índice"), "indice");
    EXPECT_EQ(profile.normalize_word("Órgano"), "organo");
    EXPECT_EQ(profile.normalize_word("Único"), "unico");
    EXPECT_EQ(profile.normalize_word("Cigüeña"), "cigueña");

    EXPECT_TRUE(profile.is_stopword("el"));
    EXPECT_TRUE(profile.is_stopword("LA"));
    EXPECT_TRUE(profile.is_stopword("por"));
    EXPECT_FALSE(profile.is_stopword("constitucion"));
    EXPECT_FALSE(profile.is_stopword("articulo"));

    EXPECT_TRUE(profile.is_abbreviation("art."));
    EXPECT_TRUE(profile.is_abbreviation("art"));
    EXPECT_TRUE(profile.is_abbreviation("sec."));
    EXPECT_FALSE(profile.is_abbreviation("constitucion"));

    EXPECT_FALSE(profile.get_stopwords().empty());
    EXPECT_FALSE(profile.get_abbreviations().empty());
}

TEST(TestLanguageProfile, EnglishProfileNormalization) {
    EnglishLanguageProfile profile;
    EXPECT_EQ(profile.get_language_code(), "en");

    EXPECT_EQ(profile.normalize_word("Constitution,"), "constitution");
    EXPECT_EQ(profile.normalize_word("ARTICLE!"), "article");

    EXPECT_TRUE(profile.is_stopword("the"));
    EXPECT_TRUE(profile.is_stopword("THE"));
    EXPECT_TRUE(profile.is_stopword("for"));
    EXPECT_FALSE(profile.is_stopword("constitution"));
    EXPECT_FALSE(profile.is_stopword("article"));

    EXPECT_TRUE(profile.is_abbreviation("sec."));
    EXPECT_TRUE(profile.is_abbreviation("art."));
    EXPECT_TRUE(profile.is_abbreviation("e.g."));
    EXPECT_FALSE(profile.is_abbreviation("constitution"));

    EXPECT_FALSE(profile.get_stopwords().empty());
    EXPECT_FALSE(profile.get_abbreviations().empty());
}

TEST(TestLanguageProfile, LanguageProfileManagerRetrieval) {
    auto& manager = LanguageProfileManager::getInstance();

    auto es_profile = manager.get_profile("es");
    ASSERT_NE(es_profile, nullptr);
    EXPECT_EQ(es_profile->get_language_code(), "es");

    auto en_profile = manager.get_profile("en");
    ASSERT_NE(en_profile, nullptr);
    EXPECT_EQ(en_profile->get_language_code(), "en");

    // Fallback to Spanish on unknown code
    auto unknown_profile = manager.get_profile("fr");
    ASSERT_NE(unknown_profile, nullptr);
    EXPECT_EQ(unknown_profile->get_language_code(), "es");

    // Active profile from configuration
    ConfigurationSystem::getInstance().set_value("COMPARISON_PROFILE", "active_language", "en");
    auto active_profile = manager.get_active_profile();
    ASSERT_NE(active_profile, nullptr);
    EXPECT_EQ(active_profile->get_language_code(), "en");

    // Reset back to "es"
    ConfigurationSystem::getInstance().set_value("COMPARISON_PROFILE", "active_language", "es");
    EXPECT_EQ(manager.get_active_profile()->get_language_code(), "es");
}

TEST(TestLanguageProfile, ManagerRegisterNull) {
    auto& manager = LanguageProfileManager::getInstance();
    manager.register_profile(nullptr);
    EXPECT_NE(manager.get_profile("es"), nullptr);
}
