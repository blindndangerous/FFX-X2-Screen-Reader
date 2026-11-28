#pragma once
#include "menu_list_text.hpp"

namespace ffx {
struct SphereUiText {
    const wchar_t* numberOption;
    const wchar_t* numberHelp;
    const wchar_t* numberPrefix;
    const wchar_t* costPrefix;
    const wchar_t* singularCost;
    const wchar_t* pluralCost;
    const wchar_t* balanceSuffix;
    bool singularAtZero=false;
};

inline const SphereUiText& sphereUiText(unsigned locale) {
    static constexpr SphereUiText labels[] = {
        {L"\u30b9\u30d5\u30a3\u30a2\u76e4\u306e\u30ce\u30fc\u30c9\u756a\u53f7", L"\u30b9\u30d5\u30a3\u30a2\u76e4\u3067\u30ce\u30fc\u30c9\u540d\u306e\u5f8c\u306b\u756a\u53f7\u3092\u8aad\u307f\u4e0a\u3052\u307e\u3059\u3002F6\u3067\u79fb\u52d5\u5148\u3092\u9078\u3076\u624b\u9806\u3067\u306f\u756a\u53f7\u3082\u6848\u5185\u3057\u307e\u3059\u3002", L"\u3001\u30ce\u30fc\u30c9", L" \u6d88\u8cbb\u30b9\u30d5\u30a3\u30a2\u30ec\u30d9\u30eb\u306f", L"\u3001\u6b8b\u308a\u306f", L"\u3001\u6b8b\u308a\u306f", L"\u3067\u3059\u3002"},
        {L"Sphere Grid node numbers", L"Say node numbers after Sphere Grid node names. Directions that require F6 always give the number to enter.", L", node ", L" Costs ", L" Sphere Level, ", L" Sphere Levels, ", L" left."},
        {L"Num\u00e9rotation des cases du sph\u00e9rier", L"Annonce les num\u00e9ros apr\u00e8s les noms des cases du sph\u00e9rier. Les indications qui demandent F6 donnent le num\u00e9ro \u00e0 saisir.", L", case ", L" Co\u00fbt : ", L" niveau de sph\u00e8re. Reste : ", L" niveaux de sph\u00e8re. Reste : ", L".", true},
        {L"N\u00famero de nodo del tablero de esferas", L"Anuncia el n\u00famero despu\u00e9s del nombre de cada nodo. Las indicaciones que requieren F6 incluyen el n\u00famero que hay que introducir.", L", nodo ", L" Coste: ", L" nivel de esfera. Restante: ", L" niveles de esfera. Restantes: ", L"."},
        {L"Feldnummern im Sph\u00e4robrett", L"Liest Feldnummern nach den Namen vor. In Wegbeschreibungen mit F6 wird die einzugebende Nummer angesagt.", L", Feld ", L" Kosten: ", L" Sph\u00e4ro-Level. Verbleibend: ", L" Sph\u00e4ro-Level. Verbleibend: ", L"."},
        {L"Numero del nodo della Sferografia", L"Legge i numeri dopo i nomi dei nodi. Le indicazioni che richiedono F6 dicono quale numero inserire.", L", nodo ", L" Costo: ", L" livello sfera. Rimanenti: ", L" livelli sfera. Rimanenti: ", L"."},
        {L"\u6676\u7403\u76d8\u8282\u70b9\u7f16\u53f7", L"\u5728\u6676\u7403\u76d8\u8282\u70b9\u540d\u79f0\u540e\u8bfb\u51fa\u7f16\u53f7\u3002\u9700\u8981\u6309F6\u7684\u6307\u5f15\u4f1a\u8bfb\u51fa\u8981\u8f93\u5165\u7684\u7f16\u53f7\u3002", L"\uff0c\u8282\u70b9", L" \u6d88\u8017", L"\u7ea7\u6676\u7403\u7b49\u7ea7\uff0c\u5269\u4f59", L"\u7ea7\u6676\u7403\u7b49\u7ea7\uff0c\u5269\u4f59", L"\u7ea7\u3002"},
        {L"\uc2a4\ud53c\uc5b4\ubc18 \ub178\ub4dc \ubc88\ud638", L"\uc2a4\ud53c\uc5b4\ubc18\uc5d0\uc11c \ub178\ub4dc \uc774\ub984 \ub4a4\uc5d0 \ubc88\ud638\ub97c \uc77d\uc2b5\ub2c8\ub2e4. F6\uc774 \ud544\uc694\ud55c \uc548\ub0b4\uc5d0\uc11c\ub294 \uc785\ub825\ud560 \ubc88\ud638\ub97c \uc54c\ub824 \uc90d\ub2c8\ub2e4.", L", \ub178\ub4dc ", L" \uc2a4\ud53c\uc5b4 \ub808\ubca8 ", L" \uc18c\ubaa8, \ub0a8\uc740 \ub808\ubca8 ", L" \uc18c\ubaa8, \ub0a8\uc740 \ub808\ubca8 ", L"."},
    };
    return labels[menuLanguageIndex(locale)];
}
}
