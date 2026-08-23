/**
 * @file dictionary_paraphrase_matcher.cpp
 * @brief Implementation of DictionaryParaphraseMatcher for Tier 1 lexical and domain paraphrase matching.
 */

#include "similarity/dictionary_paraphrase_matcher.hpp"
#include "similarity/word_sequence_aligner.hpp"
#include <algorithm>
#include <sstream>

DictionaryParaphraseMatcher::DictionaryParaphraseMatcher() {
    initialize_spanish_synonyms();
    initialize_english_synonyms();
    initialize_domain_rules();
}

void DictionaryParaphraseMatcher::add_synonym_group(const std::vector<std::string>& words, const std::string& language) {
    if (words.size() < 2) return;

    auto& word_to_cluster = (language == "en") ? m_en_word_to_cluster : m_es_word_to_cluster;
    auto& synset_clusters = (language == "en") ? m_en_synset_clusters : m_es_synset_clusters;

    std::vector<std::string> normalized_group;
    normalized_group.reserve(words.size());
    for (const auto& w : words) {
        normalized_group.push_back(WordSequenceAligner::normalize_word(w));
    }

    size_t cluster_id = synset_clusters.size();
    synset_clusters.push_back(normalized_group);

    for (const auto& w : normalized_group) {
        word_to_cluster[w] = cluster_id;
    }
}

void DictionaryParaphraseMatcher::add_domain_rule(const DomainParaphraseRule& rule) {
    DomainParaphraseRule norm_rule;
    norm_rule.ref_phrase = WordSequenceAligner::normalize_word(rule.ref_phrase);
    norm_rule.trans_phrase = WordSequenceAligner::normalize_word(rule.trans_phrase);
    norm_rule.domain_key = rule.domain_key;
    norm_rule.confidence = rule.confidence;
    m_domain_rules.push_back(norm_rule);
}

void DictionaryParaphraseMatcher::initialize_spanish_synonyms() {
    // Academic & Legal Core Verbs & Inflections
    add_synonym_group({
        "establecer", "establece", "establecen", "establecio",
        "fijar", "fija", "fijan", "fijo",
        "determinar", "determina", "determinan", "determino",
        "disponer", "dispone", "disponen", "dispuso",
        "preceptuar", "preceptua", "preceptuan", "preceptuo",
        "senalar", "senala", "senalan", "senalo",
        "estipular", "estipula", "estipulan", "estipulo"
    }, "es");

    add_synonym_group({
        "modificar", "modifica", "modifican", "modifico",
        "alterar", "altera", "alteran", "altero",
        "reformar", "reforma", "reforman", "reformo",
        "cambiar", "cambia", "cambian", "cambio",
        "variar", "varia", "varian", "vario"
    }, "es");

    add_synonym_group({
        "finalizar", "finaliza", "finalizan", "finalizo",
        "terminar", "termina", "terminan", "termino",
        "concluir", "concluye", "concluyen", "concluyo",
        "extinguir", "extingue", "extinguen", "extinguio",
        "cesar", "cesa", "cesan", "ceso",
        "acabar", "acaba", "acaban", "acabo"
    }, "es");

    add_synonym_group({
        "permitir", "permite", "permiten", "permitio",
        "autorizar", "autoriza", "autorizan", "autorizo",
        "facultar", "faculta", "facultan", "faculto",
        "habilitar", "habilita", "habilitan", "habilito",
        "consentir", "consiente", "consienten"
    }, "es");

    add_synonym_group({
        "prohibir", "prohibe", "prohiben", "prohibio",
        "vedar", "veda", "vedan",
        "impedir", "impide", "impiden", "impidio",
        "vetar", "veta", "vetan"
    }, "es");

    add_synonym_group({
        "solicitar", "solicita", "solicitan", "solicito",
        "pedir", "pide", "piden", "pidio",
        "requerir", "requiere", "requieren", "requirio",
        "reclamar", "reclama", "reclaman", "reclamo",
        "postular", "postula", "postulan",
        "demandar", "demanda", "demandan", "demando"
    }, "es");

    add_synonym_group({
        "notificar", "notifica", "notifican", "notifico",
        "comunicar", "comunica", "comunican", "comunico",
        "trasladar", "traslada", "trasladan",
        "informar", "informa", "informan", "informo"
    }, "es");

    add_synonym_group({
        "vulnerar", "vulnera", "vulneran", "vulnero",
        "infringir", "infringe", "infringen", "infringio",
        "violar", "viola", "violan", "violo",
        "quebrantar", "quebranta", "quebrantan", "quebranto",
        "incumplir", "incumple", "incumplen", "incumplio",
        "transgredir", "transgrede", "transgreden"
    }, "es");

    add_synonym_group({
        "constituir", "constituye", "constituyen", "constituyo",
        "conformar", "conforma", "conforman", "conformo",
        "integrar", "integra", "integran", "integro",
        "componer", "compone", "componen",
        "formar", "forma", "forman", "formo",
        "originar", "origina", "originan", "origino",
        "generar", "genera", "generan", "genero"
    }, "es");

    add_synonym_group({
        "regular", "regula", "regulan", "regulo",
        "normar", "norma", "norman",
        "ordenar", "ordena", "ordenan", "ordeno",
        "regir", "rige", "rigen", "rigio",
        "disciplinar", "disciplina", "disciplinan"
    }, "es");

    add_synonym_group({
        "derogar", "deroga", "derogan", "derogo",
        "abolir", "abole", "abolen",
        "anular", "anula", "anulan", "anulo",
        "revocar", "revoca", "revocan", "revoco",
        "invalidar", "invalida", "invalidan"
    }, "es");

    add_synonym_group({
        "exigir", "exige", "exigen", "exigio",
        "imponer", "impone", "imponen", "impuso"
    }, "es");

    add_synonym_group({
        "otorgar", "otorga", "otorgan", "otorgo",
        "conceder", "concede", "conceden", "concedio",
        "atribuir", "atribuye", "atribuyen", "atribuyo",
        "asignar", "asigna", "asignan", "asigno",
        "conferir", "confiere", "confieren", "confirio"
    }, "es");

    add_synonym_group({
        "acreditar", "acredita", "acreditan", "acredito",
        "demostrar", "demuestra", "demuestran", "demostro",
        "probar", "prueba", "prueban", "probo",
        "justificar", "justifica", "justifican", "justifico",
        "motivar", "motiva", "motivan", "motivo",
        "fundamentar", "fundamenta", "fundamentan", "fundamento",
        "constatar", "constata", "constatan"
    }, "es");

    // Academic & Legal Core Nouns
    add_synonym_group({"requisito", "presupuesto", "condicion", "exigencia", "premisa"}, "es");
    add_synonym_group({"tributo", "impuesto", "gravamen", "tasa", "contribucion"}, "es");
    add_synonym_group({"procedimiento", "proceso", "tramite", "actuacion"}, "es");
    add_synonym_group({"resolucion", "decision", "acuerdo", "dictamen", "fallo", "pronunciamiento"}, "es");
    add_synonym_group({"norma", "precepto", "disposicion", "regla", "ordenamiento", "ley"}, "es");
    add_synonym_group({"plazo", "termino", "periodo", "tiempo"}, "es");
    add_synonym_group({"recurso", "impugnacion", "reclamacion", "alegacion"}, "es");
    add_synonym_group({"facultad", "potestad", "poder", "competencia", "atribucion"}, "es");
    add_synonym_group({"obligacion", "deber", "compromiso", "vinculo"}, "es");
    add_synonym_group({"responsabilidad", "imputabilidad"}, "es");
    add_synonym_group({"sujeto", "persona", "individuo", "ente", "titular"}, "es");
    add_synonym_group({"finalidad", "objeto", "proposito", "objetivo", "meta", "fin"}, "es");
    add_synonym_group({"principio", "fundamento", "base", "pilar", "criterio"}, "es");

    // Adjectives & Modifiers
    add_synonym_group({"obligatorio", "preceptivo", "vinculante", "imperativo", "mandatorio", "forzoso"}, "es");
    add_synonym_group({"optativo", "facultativo", "voluntario", "discrecional"}, "es");
    add_synonym_group({"valido", "eficaz", "vigente", "legitimo"}, "es");
    add_synonym_group({"nulo", "invalido", "ineficaz", "viciado"}, "es");
    add_synonym_group({"directo", "inmediato", "expreso"}, "es");
    add_synonym_group({"indirecto", "mediato", "tacito"}, "es");
    add_synonym_group({"anterior", "previo", "precedente"}, "es");
    add_synonym_group({"posterior", "ulterior", "subsiguiente"}, "es");
}

void DictionaryParaphraseMatcher::initialize_english_synonyms() {
    add_synonym_group({
        "establish", "establishes", "established",
        "determine", "determines", "determined",
        "provide", "provides", "provided",
        "stipulate", "stipulates", "stipulated",
        "prescribe", "prescribes", "prescribed",
        "set", "sets"
    }, "en");

    add_synonym_group({
        "modify", "modifies", "modified",
        "alter", "alters", "altered",
        "amend", "amends", "amended",
        "change", "changes", "changed",
        "adjust", "adjusts", "adjusted"
    }, "en");

    add_synonym_group({
        "terminate", "terminates", "terminated",
        "conclude", "concludes", "concluded",
        "end", "ends", "ended",
        "extinguish", "extinguishes", "extinguished",
        "cease", "ceases", "ceased"
    }, "en");

    add_synonym_group({
        "permit", "permits", "permitted",
        "allow", "allows", "allowed",
        "authorize", "authorizes", "authorized",
        "grant", "grants", "granted",
        "empower", "empowers", "empowered"
    }, "en");

    add_synonym_group({
        "prohibit", "prohibits", "prohibited",
        "forbid", "forbids", "forbidden",
        "ban", "bans", "banned",
        "bar", "bars", "barred",
        "prevent", "prevents", "prevented"
    }, "en");

    add_synonym_group({
        "require", "requires", "required",
        "demand", "demands", "demanded",
        "mandate", "mandates", "mandated",
        "necessitate", "necessitates", "necessitated"
    }, "en");

    add_synonym_group({
        "notify", "notifies", "notified",
        "communicate", "communicates", "communicated",
        "inform", "informs", "informed",
        "advise", "advises", "advised"
    }, "en");

    add_synonym_group({
        "violate", "violates", "violated",
        "infringe", "infringes", "infringed",
        "breach", "breaches", "breached",
        "transgress", "transgresses", "transgressed"
    }, "en");

    add_synonym_group({
        "constitute", "constitutes", "constituted",
        "form", "forms", "formed",
        "comprise", "comprises", "comprised",
        "represent", "represents", "represented",
        "compose", "composes", "composed"
    }, "en");

    add_synonym_group({"requirement", "requirements", "condition", "conditions", "prerequisite", "prerequisites", "premise", "stipulation"}, "en");
    add_synonym_group({"mandatory", "obligatory", "compulsory", "binding", "imperative"}, "en");
    add_synonym_group({"optional", "discretionary", "voluntary", "elective"}, "en");
    add_synonym_group({"valid", "effective", "legitimate", "enforceable", "sound"}, "en");
    add_synonym_group({"resolution", "decision", "determination", "ruling", "decree"}, "en");
    add_synonym_group({"statute", "law", "act", "legislation", "regulation", "rule", "code", "provision"}, "en");
    add_synonym_group({"liability", "liabilities", "obligation", "obligations", "duty", "duties", "debt", "debts"}, "en");
}

void DictionaryParaphraseMatcher::initialize_domain_rules() {
    // Law & Administrative Domain Rules
    add_domain_rule({"hecho imponible", "presupuesto de hecho", "law", 1.0f});
    add_domain_rule({"hecho imponible", "presupuesto generador", "law", 1.0f});
    add_domain_rule({"entrar en vigor", "comenzar a regir", "law", 1.0f});
    add_domain_rule({"entrar en vigor", "producir efectos juridicos", "law", 0.95f});
    add_domain_rule({"sujeto pasivo", "obligado tributario", "law", 1.0f});
    add_domain_rule({"sujeto pasivo", "contribuyente", "law", 0.95f});
    add_domain_rule({"silencio administrativo", "falta de resolucion expresa", "law", 1.0f});
    add_domain_rule({"dejar sin efecto", "derogar", "law", 1.0f});
    add_domain_rule({"fuerza mayor", "caso fortuito", "law", 0.90f});
    add_domain_rule({"acto administrativo", "declaracion de voluntad de la administracion", "law", 0.95f});
    add_domain_rule({"recurso de alzada", "impugnacion ante el superior jerarquico", "law", 0.95f});

    // Economics Domain Rules
    add_domain_rule({"producto interior bruto", "produccion total agregada", "economics", 0.95f});
    add_domain_rule({"tipo de interes", "precio del dinero", "economics", 0.95f});
    add_domain_rule({"superavit fiscal", "excedente presupuestario", "economics", 0.95f});
    add_domain_rule({"deficit publico", "saldo presupuestario negativo", "economics", 0.95f});

    // Science Domain Rules
    add_domain_rule({"material genetico", "acido desoxirribonucleico", "science", 0.95f});
    add_domain_rule({"fuerza de atraccion", "gravedad", "science", 0.95f});

    // History Domain Rules
    add_domain_rule({"segunda republica", "regimen republicano de 1931", "history", 0.95f});
    add_domain_rule({"guerra civil", "conflicto belico espanol", "history", 0.95f});
}

bool DictionaryParaphraseMatcher::is_synonym(const std::string& word1, const std::string& word2, const std::string& language) const {
    std::string n1 = WordSequenceAligner::normalize_word(word1);
    std::string n2 = WordSequenceAligner::normalize_word(word2);

    if (n1.empty() || n2.empty()) return false;
    if (n1 == n2) return true;

    const auto& word_to_cluster = (language == "en") ? m_en_word_to_cluster : m_es_word_to_cluster;

    auto it1 = word_to_cluster.find(n1);
    auto it2 = word_to_cluster.find(n2);

    if (it1 != word_to_cluster.end() && it2 != word_to_cluster.end()) {
        return it1->second == it2->second;
    }

    return false;
}

std::vector<std::string> DictionaryParaphraseMatcher::get_synonyms(const std::string& word, const std::string& language) const {
    std::string norm = WordSequenceAligner::normalize_word(word);
    const auto& word_to_cluster = (language == "en") ? m_en_word_to_cluster : m_es_word_to_cluster;
    const auto& synset_clusters = (language == "en") ? m_en_synset_clusters : m_es_synset_clusters;

    auto it = word_to_cluster.find(norm);
    if (it != word_to_cluster.end() && it->second < synset_clusters.size()) {
        std::vector<std::string> result;
        for (const auto& w : synset_clusters[it->second]) {
            if (w != norm) {
                result.push_back(w);
            }
        }
        return result;
    }

    return {};
}

ParaphraseMatchResult DictionaryParaphraseMatcher::compare_phrases(
    const std::string& ref_phrase,
    const std::string& trans_phrase,
    const std::string& domain_key,
    const std::string& language
) const {
    ParaphraseMatchResult result;
    std::string r_norm = WordSequenceAligner::normalize_word(ref_phrase);
    std::string t_norm = WordSequenceAligner::normalize_word(trans_phrase);

    if (r_norm.empty() || t_norm.empty()) {
        return result;
    }

    // 1. Exact match (not a multi-word paraphrase)
    if (r_norm == t_norm) {
        result.is_match = true;
        result.similarity_score = 1.0f;
        result.matched_reference_phrase = ref_phrase;
        result.matched_transcript_phrase = trans_phrase;
        result.is_multi_word_phrase = false;
        return result;
    }

    // 2. Single word synonym check
    auto r_tokens = WordSequenceAligner::tokenize(r_norm);
    auto t_tokens = WordSequenceAligner::tokenize(t_norm);

    if (r_tokens.size() == 1 && t_tokens.size() == 1) {
        if (is_synonym(r_tokens[0], t_tokens[0], language)) {
            result.is_match = true;
            result.similarity_score = 0.95f;
            result.matched_reference_phrase = ref_phrase;
            result.matched_transcript_phrase = trans_phrase;
            result.ref_word_count = 1;
            result.trans_word_count = 1;
            result.is_multi_word_phrase = false;
            return result;
        }
    }

    // 3. Domain multi-word rules check
    for (const auto& rule : m_domain_rules) {
        if (domain_key == "general" || rule.domain_key == "general" || rule.domain_key == domain_key) {
            if ((rule.ref_phrase == r_norm && rule.trans_phrase == t_norm) ||
                (rule.ref_phrase == t_norm && rule.trans_phrase == r_norm)) {
                result.is_match = true;
                result.similarity_score = rule.confidence;
                result.matched_reference_phrase = ref_phrase;
                result.matched_transcript_phrase = trans_phrase;
                result.ref_word_count = r_tokens.size();
                result.trans_word_count = t_tokens.size();
                result.is_multi_word_phrase = true;
                return result;
            }
        }
    }

    return result;
}

std::vector<ParaphraseMatchResult> DictionaryParaphraseMatcher::find_paraphrases(
    const std::vector<std::string>& ref_words,
    const std::vector<std::string>& trans_words,
    const std::string& domain_key,
    const std::string& language
) const {
    std::vector<ParaphraseMatchResult> results;
    if (ref_words.empty() || trans_words.empty()) return results;

    std::vector<bool> ref_used(ref_words.size(), false);
    std::vector<bool> trans_used(trans_words.size(), false);

    // 1. Multi-word domain phrase scanning (from 4 words down to 2 words)
    for (size_t r_len = 4; r_len >= 2; --r_len) {
        for (size_t ri = 0; ri + r_len <= ref_words.size(); ++ri) {
            bool ref_span_free = true;
            for (size_t k = 0; k < r_len; ++k) {
                if (ref_used[ri + k]) { ref_span_free = false; break; }
            }
            if (!ref_span_free) continue;

            std::string ref_phrase;
            for (size_t k = 0; k < r_len; ++k) {
                if (k > 0) ref_phrase += " ";
                ref_phrase += ref_words[ri + k];
            }

            for (size_t t_len = 5; t_len >= 1; --t_len) {
                for (size_t ti = 0; ti + t_len <= trans_words.size(); ++ti) {
                    bool trans_span_free = true;
                    for (size_t k = 0; k < t_len; ++k) {
                        if (trans_used[ti + k]) { trans_span_free = false; break; }
                    }
                    if (!trans_span_free) continue;

                    std::string trans_phrase;
                    for (size_t k = 0; k < t_len; ++k) {
                        if (k > 0) trans_phrase += " ";
                        trans_phrase += trans_words[ti + k];
                    }

                    auto match = compare_phrases(ref_phrase, trans_phrase, domain_key, language);
                    if (match.is_match && match.is_multi_word_phrase) {
                        match.ref_start_index = ri;
                        match.ref_word_count = r_len;
                        match.trans_start_index = ti;
                        match.trans_word_count = t_len;

                        for (size_t k = 0; k < r_len; ++k) ref_used[ri + k] = true;
                        for (size_t k = 0; k < t_len; ++k) trans_used[ti + k] = true;

                        results.push_back(match);
                        break;
                    }
                }
            }
        }
    }

    // 2. Single-word synonym scanning for remaining unused tokens
    for (size_t ri = 0; ri < ref_words.size(); ++ri) {
        if (ref_used[ri] || ref_words[ri].empty()) continue;

        for (size_t ti = 0; ti < trans_words.size(); ++ti) {
            if (trans_used[ti] || trans_words[ti].empty()) continue;

            if (ref_words[ri] != trans_words[ti] && is_synonym(ref_words[ri], trans_words[ti], language)) {
                ParaphraseMatchResult match;
                match.is_match = true;
                match.similarity_score = 0.95f;
                match.matched_reference_phrase = ref_words[ri];
                match.matched_transcript_phrase = trans_words[ti];
                match.ref_start_index = ri;
                match.ref_word_count = 1;
                match.trans_start_index = ti;
                match.trans_word_count = 1;
                match.is_multi_word_phrase = false;

                ref_used[ri] = true;
                trans_used[ti] = true;
                results.push_back(match);
                break;
            }
        }
    }

    return results;
}
