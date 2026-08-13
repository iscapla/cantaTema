/**
 * @file t_domain_profile.cxx
 * @brief Unit tests for IDomainProfile, concrete domain profiles, and DomainProfileManager.
 */

#include <gtest/gtest.h>
#include "primitives/domain_profiles.hpp"
#include "primitives/domain_profile_manager.hpp"
#include "configuration/configuration_system.hpp"

TEST(TestDomainProfile, LawDomainProfileTests) {
    LawDomainProfile profile;
    EXPECT_EQ(profile.get_domain_key(), "law");
    EXPECT_FALSE(profile.get_domain_name().empty());
    EXPECT_EQ(profile.get_missing_warning_badge(), "⚠️ MISSING LEGAL CITATION");

    EXPECT_TRUE(profile.is_high_priority_citation("Artículo"));
    EXPECT_TRUE(profile.is_high_priority_citation("Ley"));
    EXPECT_TRUE(profile.is_high_priority_citation("RD"));
    EXPECT_FALSE(profile.is_high_priority_citation("historia"));

    EXPECT_FLOAT_EQ(profile.get_token_weight("Artículo", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("35/2006", false), 3.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("Estatuto", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("norma", false), 1.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("de", true), 0.2f);
}

TEST(TestDomainProfile, EconomicsDomainProfileTests) {
    EconomicsDomainProfile profile;
    EXPECT_EQ(profile.get_domain_key(), "economics");
    EXPECT_FALSE(profile.get_domain_name().empty());
    EXPECT_EQ(profile.get_missing_warning_badge(), "⚠️ MISSING ECONOMIC INDICATOR");

    EXPECT_TRUE(profile.is_high_priority_citation("PIB"));
    EXPECT_TRUE(profile.is_high_priority_citation("IPC"));
    EXPECT_TRUE(profile.is_high_priority_citation("15%"));
    EXPECT_TRUE(profile.is_high_priority_citation("euros"));
    EXPECT_FALSE(profile.is_high_priority_citation("ciudad"));

    EXPECT_FLOAT_EQ(profile.get_token_weight("PIB", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("1000", false), 3.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("Mercado", false), 2.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("el", true), 0.2f);
}

TEST(TestDomainProfile, HistoryDomainProfileTests) {
    HistoryDomainProfile profile;
    EXPECT_EQ(profile.get_domain_key(), "history");
    EXPECT_FALSE(profile.get_domain_name().empty());
    EXPECT_EQ(profile.get_missing_warning_badge(), "⚠️ MISSING HISTORICAL ERA/DATE");

    EXPECT_TRUE(profile.is_high_priority_citation("Siglo"));
    EXPECT_TRUE(profile.is_high_priority_citation("Tratado"));
    EXPECT_TRUE(profile.is_high_priority_citation("1492"));
    EXPECT_FALSE(profile.is_high_priority_citation("algoritmo"));

    EXPECT_FLOAT_EQ(profile.get_token_weight("1492", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("Tratado", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("Versalles", false), 3.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("de", true), 0.2f);
}

TEST(TestDomainProfile, ScienceDomainProfileTests) {
    ScienceDomainProfile profile;
    EXPECT_EQ(profile.get_domain_key(), "science");
    EXPECT_FALSE(profile.get_domain_name().empty());
    EXPECT_EQ(profile.get_missing_warning_badge(), "⚠️ MISSING SCIENTIFIC TERM/UNIT");

    EXPECT_TRUE(profile.is_high_priority_citation("ADN"));
    EXPECT_TRUE(profile.is_high_priority_citation("ATP"));
    EXPECT_TRUE(profile.is_high_priority_citation("kg"));
    EXPECT_TRUE(profile.is_high_priority_citation("mol"));
    EXPECT_FALSE(profile.is_high_priority_citation("parrafo"));

    EXPECT_FLOAT_EQ(profile.get_token_weight("ADN", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("9.8", false), 3.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("Mitocondria", false), 4.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("Hipotesis", false), 2.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("en", true), 0.2f);

}

TEST(TestDomainProfile, GeneralDomainProfileTests) {
    GeneralDomainProfile profile;
    EXPECT_EQ(profile.get_domain_key(), "general");
    EXPECT_FALSE(profile.get_domain_name().empty());
    EXPECT_EQ(profile.get_missing_warning_badge(), "⚠️ MISSING KEYWORD");

    EXPECT_TRUE(profile.is_high_priority_citation("Shakespeare"));
    EXPECT_TRUE(profile.is_high_priority_citation("100"));
    EXPECT_FALSE(profile.is_high_priority_citation("texto"));

    EXPECT_FLOAT_EQ(profile.get_token_weight("Shakespeare", false), 3.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("texto", false), 1.0f);
    EXPECT_FLOAT_EQ(profile.get_token_weight("la", true), 0.2f);
}

TEST(TestDomainProfile, DomainProfileManagerRetrieval) {
    auto& manager = DomainProfileManager::getInstance();

    auto law = manager.get_profile("law");
    ASSERT_NE(law, nullptr);
    EXPECT_EQ(law->get_domain_key(), "law");

    auto eco = manager.get_profile("economics");
    ASSERT_NE(eco, nullptr);
    EXPECT_EQ(eco->get_domain_key(), "economics");

    auto hist = manager.get_profile("history");
    ASSERT_NE(hist, nullptr);
    EXPECT_EQ(hist->get_domain_key(), "history");

    auto sci = manager.get_profile("science");
    ASSERT_NE(sci, nullptr);
    EXPECT_EQ(sci->get_domain_key(), "science");

    auto gen = manager.get_profile("general");
    ASSERT_NE(gen, nullptr);
    EXPECT_EQ(gen->get_domain_key(), "general");

    // Fallback to general on unknown domain
    auto unknown = manager.get_profile("quantum_physics");
    ASSERT_NE(unknown, nullptr);
    EXPECT_EQ(unknown->get_domain_key(), "general");

    // Active profile from configuration
    ConfigurationSystem::getInstance().set_value("COMPARISON_PROFILE", "active_domain", "history");
    auto active = manager.get_active_profile();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->get_domain_key(), "history");

    // Reset back to "general"
    ConfigurationSystem::getInstance().set_value("COMPARISON_PROFILE", "active_domain", "general");
    EXPECT_EQ(manager.get_active_profile()->get_domain_key(), "general");
}

TEST(TestDomainProfile, ManagerRegisterNull) {
    auto& manager = DomainProfileManager::getInstance();
    manager.register_profile(nullptr);
    EXPECT_NE(manager.get_profile("law"), nullptr);
}
