#pragma once
#include "menu_list_text.hpp"

namespace ffx {
struct ContextHelpText {
    const wchar_t *label, *settings, *planner, *reels, *naming, *battle, *grid,
        *menu, *cloister, *via, *gagazet, *field, *sensor, *shop, *review, *footer;
};

inline const ContextHelpText& contextHelpText(unsigned locale) {
    static constexpr ContextHelpText catalog[]={
        {
            L"\u30d8\u30eb\u30d7",
            L"\u8a2d\u5b9a\u30e1\u30cb\u30e5\u30fc\u3002\u4e0a\u4e0b\u3067\u9078\u629e\u3001\u5de6\u53f3\u3067\u5909\u66f4\u3002Enter\u307e\u305f\u306fA\u3067\u6c7a\u5b9a\u3002I\u307e\u305f\u306fX\u3067\u8aac\u660e\u3002Home\u3068End\u3067\u5148\u982d\u3068\u672b\u5c3e\u3002Esc\u307e\u305f\u306fB\u3067\u623b\u308b\u3002F8\u3067\u9589\u3058\u307e\u3059\u3002",
            L"\u30b9\u30d5\u30a3\u30a2\u76e4\u30d7\u30e9\u30f3\u30ca\u30fc\u3002\u4e0a\u4e0b\u3067\u9078\u629e\u3001Page Up\u3068Page Down\u3067\u79fb\u52d5\u3002Enter\u3067\u6c7a\u5b9a\u3001I\u3067\u8aac\u660e\u3001Esc\u3067\u9589\u3058\u307e\u3059\u3002",
            L"\u30ef\u30c3\u30ab\u306e\u30ea\u30fc\u30eb\u3002\u4e0a\u4e0b\u3067\u9078\u629e\u3001Enter\u307e\u305f\u306fA\u3067\u6c7a\u5b9a\u3002I\u3067\u8aac\u660e\u3001Esc\u307e\u305f\u306fB\u3067\u624b\u52d5\u64cd\u4f5c\u306b\u623b\u308a\u307e\u3059\u3002",
            L"\u6587\u5b57\u5165\u529b\u3002F11\u3067\u5165\u529b\u3057\u305f\u6587\u5b57\u3092\u8aad\u307f\u4e0a\u3052\u307e\u3059\u3002",
            L"\u30d0\u30c8\u30eb\u3002T\u3067\u73fe\u5728\u306e\u30bf\u30fc\u30f3\u3001H\u3067HP\u3068\u72b6\u614b\u3001O\u3067\u30aa\u30fc\u30d0\u30fc\u30c9\u30e9\u30a4\u30d6\u3002Shift+T\u3067\u884c\u52d5\u9806\u3001Shift+H\u3067\u8868\u793a\u4e2d\u306eHP\u3068MP\u3001Shift+O\u3067\u30b2\u30fc\u30b8\u3002I\u3067\u8a73\u7d30\u3001F11\u3067\u7e70\u308a\u8fd4\u3057\u3001F8\u3067\u8a2d\u5b9a\u3002",
            L"\u30b9\u30d5\u30a3\u30a2\u76e4\u3002F2\u3067\u7d4c\u8def\u3001F3\u3067\u30a2\u30d3\u30ea\u30c6\u30a3\u3001Shift+F2\u3067\u89e3\u9664\u3002F4\u3068F5\u3067\u524d\u5f8c\u306e\u6848\u5185\u3002\u30d0\u30c3\u30af\u30b9\u30e9\u30c3\u30b7\u30e5\u3068Shift+\u30d0\u30c3\u30af\u30b9\u30e9\u30c3\u30b7\u30e5\u3067\u30ab\u30fc\u30bd\u30eb\u304b\u3089\u6848\u5185\u3002O\u3067\u4f4d\u7f6e\u3001S\u3067\u30b9\u30d5\u30a3\u30a2\u30ec\u30d9\u30eb\u3001F6\u3067\u30ce\u30fc\u30c9\u756a\u53f7\u3001I\u3067\u8aac\u660e\u3001F7\u3067\u81ea\u52d5\u8a73\u7d30\u3001F8\u3067\u8a2d\u5b9a\u3002",
            L"\u30b2\u30fc\u30e0\u30e1\u30cb\u30e5\u30fc\u3002I\u3067\u8a73\u7d30\u3001F7\u3067\u81ea\u52d5\u8a73\u7d30\u3001F11\u3067\u7e70\u308a\u8fd4\u3057\u3001F8\u3067\u8a2d\u5b9a\u3002",
            L"\u8a66\u7df4\u306e\u9593\u3002F9\u3068F10\u3067\u524d\u5f8c\u306e\u624b\u9806\u3002Shift+F10\u3067\u9078\u629e\u4e2d\u3001F11\u3067\u73fe\u5728\u3001Shift+F11\u3067\u63a8\u5b9a\u624b\u9806\u3002F8\u3001Miscellaneous\u3001Cloister of Trials\u3002",
            L"\u6d44\u7f6a\u306e\u8def\u3002F9\u3068F10\u3067\u524d\u5f8c\u306e\u624b\u9806\u3002Shift+F10\u3067\u9078\u629e\u4e2d\u3001F11\u3067\u73fe\u5728\u3001Shift+F11\u3067\u63a8\u5b9a\u624b\u9806\u3002F8\u3001Miscellaneous\u3001Via Purifico\u3002",
            L"\u30ac\u30ac\u30bc\u30c8\u5c71\u306e\u6d1e\u7a9f\u3002F11\u3067\u6848\u5185\u3001Shift+F11\u3067\u9078\u629e\u4e2d\u306e\u7a74\u3002F8\u3001Miscellaneous\u3001Gagazet cave puzzles\u3002",
            L"\u30d5\u30a3\u30fc\u30eb\u30c9\u3002\u30d0\u30c3\u30af\u30af\u30a9\u30fc\u30c8\u3067\u518d\u691c\u7d22\u3001\u89d2\u62ec\u5f27\u3067\u76ee\u7684\u5730\u3001Shift\u3068\u89d2\u62ec\u5f27\u3067\u5206\u985e\u3001P\u3067\u76ee\u7684\u5730\u3092\u518d\u8aad\u3002\u30d0\u30c3\u30af\u30b9\u30e9\u30c3\u30b7\u30e5\u3067\u7d4c\u8def\u3001U\u3067\u5230\u9054\u53ef\u80fd\u306a\u5834\u6240\u306e\u7d5e\u308a\u8fbc\u307f\u3001G\u3067\u30ae\u30eb\u3001M\u3067\u5730\u56f3\u540d\u3002F6\u3067\u8a98\u5c0e\u97f3\u3001F7\u3067\u81ea\u52d5\u8a73\u7d30\u3001F8\u3067\u8a2d\u5b9a\u3002",
            L"Ctrl\u3068\u77e2\u5370\u30ad\u30fc\u3067\u307f\u3084\u3076\u308b\u60c5\u5831\u3002", L"U\u3067\u8ffd\u52a0\u60c5\u5831\u3002", L"\u30b3\u30f3\u30de\u3068\u30d4\u30ea\u30aa\u30c9\u3067\u524d\u5f8c\u306e\u884c\u3002", L"Shift+I\u3067\u8868\u793a\u4e2d\u306e\u64cd\u4f5c\u8aac\u660e\u3002"
        },
        {
            L"Help",
            L"Mod menu. Up and Down select. Left and Right change. Enter or A activates. I or X describes. Home and End jump to the first and last row. Escape or B goes back. F8 closes.",
            L"Sphere Grid planner. Up and Down browse. Page Up and Page Down jump. Enter chooses. I describes. Escape closes.",
            L"Wakka's reels. Up and Down choose. Enter or A applies. I explains. Escape or B returns to manual reels.",
            L"Text entry. F11 reads the entered text.",
            L"Battle. T reads the current turn, H reads HP and status, and O reads Overdrive. Shift+T reads turn order, Shift+H reads visible HP and MP, and Shift+O reads visible meters. I reads details. F11 repeats. F8 opens the mod menu.",
            L"Sphere Grid. F2 plans a path, F3 plans an ability, and Shift+F2 clears plans. F4 and F5 guide forward and back. Backslash and Shift+Backslash guide from the cursor. O reads orientation, S reads Sphere Levels, and F6 takes a node number. I reads descriptions, F7 toggles Autodetail, and F8 opens the mod menu.",
            L"Game menu. I reads details. F7 toggles Autodetail. F11 repeats. F8 opens the mod menu.",
            L"Cloister of Trials. F9 and F10 move between steps. Shift+F10 reads the selected step. F11 reads the current step, and Shift+F11 reads the inferred step. Open Cloister of Trials under F8, Miscellaneous.",
            L"Via Purifico. F9 and F10 move between steps. Shift+F10 reads the selected step. F11 reads the current step, and Shift+F11 reads the inferred step. Open Via Purifico under F8, Miscellaneous.",
            L"Gagazet cave puzzles. F11 reads instructions. Shift+F11 reads the selected opening during a puzzle. Open Gagazet cave puzzles under F8, Miscellaneous.",
            L"Field. Backtick rescans. Brackets select destinations. Shift+brackets change category. P repeats the destination. Backslash gives directions. U filters unreachable destinations. G reads gil and M reads the map name. F6 toggles the beacon, F7 toggles Autodetail, and F8 opens the mod menu.",
            L"Control with arrows reviews Sensor information.", L"U reads extra information.", L"Comma and period review lines.", L"Shift+I reads displayed controls."
        },
        {
            L"Aide",
            L"Menu du mod. Haut et Bas s\u00e9lectionnent. Gauche et Droite modifient. Entr\u00e9e ou A active. I ou X d\u00e9crit. D\u00e9but et Fin sautent au premier ou au dernier \u00e9l\u00e9ment. \u00c9chap ou B revient. F8 ferme le menu.",
            L"Planificateur du sph\u00e9rier. Haut et Bas parcourent la liste. Page pr\u00e9c\u00e9dente et Page suivante sautent. Entr\u00e9e choisit, I d\u00e9crit et \u00c9chap ferme.",
            L"Rouleaux de Wakka. Haut et Bas s\u00e9lectionnent. Entr\u00e9e ou A applique. I explique. \u00c9chap ou B reprend les rouleaux manuels.",
            L"Saisie de texte. F11 lit le texte saisi.",
            L"Combat. T annonce le tour, H les PV et l'\u00e9tat, O l'Overdrive. Maj+T lit l'ordre, Maj+H les PV et PM affich\u00e9s, Maj+O les jauges affich\u00e9es. I lit les d\u00e9tails, F11 r\u00e9p\u00e8te et F8 ouvre le menu du mod.",
            L"Sph\u00e9rier. F2 planifie un chemin, F3 une comp\u00e9tence et Maj+F2 efface les plans. F4 et F5 guident en avant et en arri\u00e8re. La barre oblique inverse, avec ou sans Maj, guide depuis le curseur. O annonce la position, S les niveaux et F6 saisit un num\u00e9ro. I d\u00e9crit, F7 r\u00e8gle la lecture automatique et F8 ouvre le menu du mod.",
            L"Menu du jeu. I lit les d\u00e9tails, F7 r\u00e8gle la lecture automatique, F11 r\u00e9p\u00e8te et F8 ouvre le menu du mod.",
            L"Salle de l'\u00e9preuve. F9 et F10 changent d'\u00e9tape. Maj+F10 lit l'\u00e9tape s\u00e9lectionn\u00e9e. F11 lit l'\u00e9tape actuelle, Maj+F11 l'\u00e9tape d\u00e9duite. Dans F8, ouvrez Miscellaneous puis Cloister of Trials.",
            L"Via Purifico. F9 et F10 changent d'\u00e9tape. Maj+F10 lit l'\u00e9tape s\u00e9lectionn\u00e9e. F11 lit l'\u00e9tape actuelle, Maj+F11 l'\u00e9tape d\u00e9duite. Dans F8, ouvrez Miscellaneous puis Via Purifico.",
            L"\u00c9nigmes du mont Gagazet. F11 lit les instructions. Maj+F11 indique l'ouverture s\u00e9lectionn\u00e9e pendant une \u00e9preuve. Dans F8, ouvrez Miscellaneous puis Gagazet cave puzzles.",
            L"Exploration. L'accent grave relance la recherche. Les crochets changent de destination, Maj avec les crochets change de cat\u00e9gorie, P r\u00e9p\u00e8te la destination. La barre oblique inverse donne le chemin. U filtre les destinations inaccessibles, G lit les gils et M la carte. F6 r\u00e8gle la balise, F7 la lecture automatique, F8 ouvre le menu.",
            L"Ctrl avec les fl\u00e8ches parcourt les informations de Scan.", L"U lit les informations suppl\u00e9mentaires.", L"La virgule et le point parcourent les lignes.", L"Maj+I lit les commandes affich\u00e9es."
        },
        {
            L"Ayuda",
            L"Men\u00fa del mod. Arriba y Abajo seleccionan. Izquierda y Derecha cambian valores. Intro o A activa. I o X describe. Inicio y Fin saltan a los extremos. Escape o B vuelve. F8 cierra.",
            L"Planificador del tablero de esferas. Arriba y Abajo recorren la lista. Re P\u00e1g y Av P\u00e1g saltan. Intro elige, I describe y Escape cierra.",
            L"Ruletas de Wakka. Arriba y Abajo eligen. Intro o A aplica. I explica. Escape o B vuelve a las ruletas manuales.",
            L"Entrada de texto. F11 lee el texto escrito.",
            L"Combate. T lee el turno, H la vitalidad y el estado, y O el Turbo. May\u00fas+T lee el orden, May\u00fas+H la vitalidad y los PM visibles, y May\u00fas+O los indicadores visibles. I lee detalles, F11 repite y F8 abre el men\u00fa del mod.",
            L"Tablero de esferas. F2 planifica una ruta, F3 una habilidad y May\u00fas+F2 borra los planes. F4 y F5 gu\u00edan adelante y atr\u00e1s. La barra inversa, con o sin May\u00fas, gu\u00eda desde el cursor. O indica la orientaci\u00f3n, S los niveles y F6 permite introducir un n\u00famero. I describe, F7 activa o desactiva el detalle autom\u00e1tico y F8 abre el men\u00fa.",
            L"Men\u00fa del juego. I lee los detalles. F7 activa o desactiva el detalle autom\u00e1tico. F11 repite. F8 abre el men\u00fa del mod.",
            L"Recinto de la prueba. F9 y F10 cambian de paso. May\u00fas+F10 lee el paso seleccionado. F11 lee el paso actual y May\u00fas+F11 el paso deducido. En F8, abre Miscellaneous y despu\u00e9s Cloister of Trials.",
            L"V\u00eda Purifico. F9 y F10 cambian de paso. May\u00fas+F10 lee el paso seleccionado. F11 lee el paso actual y May\u00fas+F11 el paso deducido. En F8, abre Miscellaneous y despu\u00e9s Via Purifico.",
            L"Puzles del monte Gagazet. F11 lee las instrucciones. May\u00fas+F11 lee la abertura seleccionada durante la prueba. En F8, abre Miscellaneous y despu\u00e9s Gagazet cave puzzles.",
            L"Exploraci\u00f3n. El acento grave vuelve a buscar. Los corchetes cambian el destino, May\u00fas con corchetes cambia la categor\u00eda, y P repite el destino. La barra inversa da instrucciones. U filtra destinos inaccesibles, G lee los guiles y M el mapa. F6 cambia la baliza, F7 el detalle autom\u00e1tico y F8 abre el men\u00fa.",
            L"Control con las flechas revisa la informaci\u00f3n de Libra.", L"U lee informaci\u00f3n adicional.", L"Coma y punto recorren las l\u00edneas.", L"May\u00fas+I lee los controles visibles."
        },
        {
            L"Hilfe",
            L"Mod-Men\u00fc. Hoch und Runter w\u00e4hlen. Links und Rechts \u00e4ndern Werte. Eingabe oder A aktiviert. I oder X beschreibt. Pos1 und Ende springen. Escape oder B geht zur\u00fcck. F8 schlie\u00dft.",
            L"Sph\u00e4robrett-Planer. Hoch und Runter bl\u00e4ttern. Bild auf und Bild ab springen. Eingabe w\u00e4hlt, I beschreibt und Escape schlie\u00dft.",
            L"Wakkas Walzen. Hoch und Runter w\u00e4hlen. Eingabe oder A best\u00e4tigt. I erkl\u00e4rt. Escape oder B kehrt zur manuellen Steuerung zur\u00fcck.",
            L"Texteingabe. F11 liest den eingegebenen Text.",
            L"Kampf. T liest den aktiven Zug, H HP und Zustand und O Ekstase. Umschalt+T liest die Zugfolge, Umschalt+H sichtbare HP und MP und Umschalt+O sichtbare Leisten. I liest Details, F11 wiederholt und F8 \u00f6ffnet das Mod-Men\u00fc.",
            L"Sph\u00e4robrett. F2 plant einen Weg, F3 eine F\u00e4higkeit und Umschalt+F2 l\u00f6scht Pl\u00e4ne. F4 und F5 f\u00fchren vor oder zur\u00fcck. Der R\u00fcckstrich mit oder ohne Umschalt f\u00fchrt ab dem Cursor. O liest die Position, S die Sph\u00e4ro-Level und F6 nimmt eine Feldnummer an. I beschreibt, F7 schaltet automatische Details und F8 \u00f6ffnet das Men\u00fc.",
            L"Spielmen\u00fc. I liest Details, F7 schaltet automatische Details, F11 wiederholt und F8 \u00f6ffnet das Mod-Men\u00fc.",
            L"Halle der Pr\u00fcfungen. F9 und F10 wechseln den Schritt. Umschalt+F10 liest den gew\u00e4hlten Schritt. F11 liest den aktuellen und Umschalt+F11 den ermittelten Schritt. \u00d6ffne \u00fcber F8, Miscellaneous, Cloister of Trials.",
            L"Via Purifico. F9 und F10 wechseln den Schritt. Umschalt+F10 liest den gew\u00e4hlten Schritt. F11 liest den aktuellen und Umschalt+F11 den ermittelten Schritt. \u00d6ffne \u00fcber F8, Miscellaneous, Via Purifico.",
            L"R\u00e4tsel am Berg Gagazet. F11 liest die Anleitung. Umschalt+F11 liest w\u00e4hrend des R\u00e4tsels die gew\u00e4hlte \u00d6ffnung. \u00d6ffne \u00fcber F8, Miscellaneous, Gagazet cave puzzles.",
            L"Erkundung. Das Graviszeichen sucht erneut. Eckige Klammern w\u00e4hlen Ziele, Umschalt mit Klammern Kategorien und P wiederholt das Ziel. Der R\u00fcckstrich gibt den Weg an. U filtert unerreichbare Ziele, G liest Gil, M den Kartennamen. F6 schaltet das Tonsignal, F7 automatische Details und F8 \u00f6ffnet das Men\u00fc.",
            L"Strg mit Pfeiltasten liest Analyse-Informationen.", L"U liest Zusatzinformationen.", L"Komma und Punkt bl\u00e4ttern durch die Zeilen.", L"Umschalt+I liest die angezeigten Tasten."
        },
        {
            L"Aiuto",
            L"Menu della mod. Su e Gi\u00f9 selezionano. Sinistra e Destra modificano. Invio o A attiva. I o X descrive. Inizio e Fine saltano agli estremi. Esc o B torna indietro. F8 chiude.",
            L"Pianificatore della Sferografia. Su e Gi\u00f9 scorrono. Pagina su e Pagina gi\u00f9 saltano. Invio sceglie, I descrive ed Esc chiude.",
            L"Slot di Wakka. Su e Gi\u00f9 scelgono. Invio o A conferma. I spiega. Esc o B torna alle slot manuali.",
            L"Inserimento testo. F11 legge il testo inserito.",
            L"Battaglia. T legge il turno, H gli HP e lo stato, O la Turbotecnica. Maiusc+T legge l'ordine, Maiusc+H gli HP e MP visibili, Maiusc+O le barre visibili. I legge i dettagli, F11 ripete e F8 apre il menu.",
            L"Sferografia. F2 pianifica un percorso, F3 un'abilit\u00e0 e Maiusc+F2 cancella i piani. F4 e F5 guidano avanti e indietro. La barra inversa, con o senza Maiusc, guida dal cursore. O legge la posizione, S i livelli sfera e F6 accetta un numero. I descrive, F7 commuta i dettagli automatici e F8 apre il menu.",
            L"Menu di gioco. I legge i dettagli, F7 commuta i dettagli automatici, F11 ripete e F8 apre il menu della mod.",
            L"Chiostro della prova. F9 e F10 cambiano passo. Maiusc+F10 legge il passo selezionato. F11 legge il passo corrente e Maiusc+F11 quello dedotto. In F8, apri Miscellaneous e poi Cloister of Trials.",
            L"Via Purificatio. F9 e F10 cambiano passo. Maiusc+F10 legge il passo selezionato. F11 legge il passo corrente e Maiusc+F11 quello dedotto. In F8, apri Miscellaneous e poi Via Purifico.",
            L"Enigmi del Gagazet. F11 legge le istruzioni. Maiusc+F11 legge l'apertura selezionata durante la prova. In F8, apri Miscellaneous e poi Gagazet cave puzzles.",
            L"Esplorazione. L'accento grave ripete la ricerca. Le parentesi quadre scelgono la meta, Maiusc con le parentesi cambia categoria e P ripete la meta. La barra inversa indica il percorso. U filtra le mete irraggiungibili, G legge i guil e M la mappa. F6 commuta il segnale, F7 i dettagli automatici e F8 apre il menu.",
            L"Ctrl con le frecce legge le informazioni di Scan.", L"U legge le informazioni aggiuntive.", L"Virgola e punto scorrono le righe.", L"Maiusc+I legge i comandi visibili."
        },
        {
            L"\u5e2e\u52a9",
            L"\u6a21\u7ec4\u83dc\u5355\u3002\u4e0a\u4e0b\u9009\u62e9\uff0c\u5de6\u53f3\u66f4\u6539\u3002Enter\u6216A\u786e\u8ba4\uff0cI\u6216X\u8bfb\u8bf4\u660e\u3002Home\u548cEnd\u8df3\u5230\u9996\u5c3e\u3002Esc\u6216B\u8fd4\u56de\uff0cF8\u5173\u95ed\u3002",
            L"\u6676\u7403\u76d8\u89c4\u5212\u3002\u4e0a\u4e0b\u6d4f\u89c8\uff0cPage Up\u548cPage Down\u8df3\u9875\u3002Enter\u9009\u62e9\uff0cI\u8bfb\u8bf4\u660e\uff0cEsc\u5173\u95ed\u3002",
            L"\u74e6\u5361\u7684\u8f6c\u8f6e\u3002\u4e0a\u4e0b\u9009\u62e9\uff0cEnter\u6216A\u786e\u8ba4\uff0cI\u8bfb\u8bf4\u660e\uff0cEsc\u6216B\u8fd4\u56de\u624b\u52a8\u8f6c\u8f6e\u3002",
            L"\u6587\u5b57\u8f93\u5165\u3002F11\u6717\u8bfb\u5df2\u8f93\u5165\u7684\u6587\u5b57\u3002",
            L"\u6218\u6597\u3002T\u8bfb\u5f53\u524d\u56de\u5408\uff0cH\u8bfb\u751f\u547d\u503c\u548c\u72b6\u6001\uff0cO\u8bfb\u6781\u9650\u6280\u69fd\u3002Shift+T\u8bfb\u884c\u52a8\u987a\u5e8f\uff0cShift+H\u8bfb\u663e\u793a\u7684\u751f\u547d\u503c\u548c\u9b54\u529b\u503c\uff0cShift+O\u8bfb\u663e\u793a\u7684\u69fd\u3002I\u8bfb\u8be6\u60c5\uff0cF11\u91cd\u590d\uff0cF8\u6253\u5f00\u6a21\u7ec4\u83dc\u5355\u3002",
            L"\u6676\u7403\u76d8\u3002F2\u89c4\u5212\u8def\u7ebf\uff0cF3\u89c4\u5212\u6280\u80fd\uff0cShift+F2\u6e05\u9664\u89c4\u5212\u3002F4\u548cF5\u5411\u524d\u6216\u5411\u540e\u6307\u5f15\u3002\u53cd\u659c\u6760\u548cShift\u52a0\u53cd\u659c\u6760\u4ece\u5149\u6807\u4f4d\u7f6e\u6307\u5f15\u3002O\u8bfb\u65b9\u4f4d\uff0cS\u8bfb\u6676\u7403\u7b49\u7ea7\uff0cF6\u8f93\u5165\u8282\u70b9\u7f16\u53f7\u3002I\u8bfb\u8bf4\u660e\uff0cF7\u5207\u6362\u81ea\u52a8\u8be6\u60c5\uff0cF8\u6253\u5f00\u6a21\u7ec4\u83dc\u5355\u3002",
            L"\u6e38\u620f\u83dc\u5355\u3002I\u8bfb\u8be6\u60c5\uff0cF7\u5207\u6362\u81ea\u52a8\u8be6\u60c5\uff0cF11\u91cd\u590d\uff0cF8\u6253\u5f00\u6a21\u7ec4\u83dc\u5355\u3002",
            L"\u8bd5\u70bc\u4e4b\u95f4\u3002F9\u548cF10\u5207\u6362\u6b65\u9aa4\uff0cShift+F10\u8bfb\u6240\u9009\u6b65\u9aa4\uff0cF11\u8bfb\u5f53\u524d\u6b65\u9aa4\uff0cShift+F11\u8bfb\u63a8\u65ad\u6b65\u9aa4\u3002\u7528F8\u6253\u5f00Miscellaneous\uff0c\u518d\u6253\u5f00Cloister of Trials\u3002",
            L"\u51c0\u7f6a\u4e4b\u8def\u3002F9\u548cF10\u5207\u6362\u6b65\u9aa4\uff0cShift+F10\u8bfb\u6240\u9009\u6b65\u9aa4\uff0cF11\u8bfb\u5f53\u524d\u6b65\u9aa4\uff0cShift+F11\u8bfb\u63a8\u65ad\u6b65\u9aa4\u3002\u7528F8\u6253\u5f00Miscellaneous\uff0c\u518d\u6253\u5f00Via Purifico\u3002",
            L"\u560e\u560e\u672d\u7279\u5c71\u6d1e\u8c1c\u9898\u3002F11\u8bfb\u8bf4\u660e\u3002\u89e3\u8c1c\u65f6\uff0cShift+F11\u8bfb\u6240\u9009\u6d1e\u53e3\u3002\u7528F8\u6253\u5f00Miscellaneous\uff0c\u518d\u6253\u5f00Gagazet cave puzzles\u3002",
            L"\u63a2\u7d22\u3002\u53cd\u5f15\u53f7\u91cd\u65b0\u626b\u63cf\uff0c\u65b9\u62ec\u53f7\u9009\u62e9\u76ee\u7684\u5730\uff0cShift\u52a0\u65b9\u62ec\u53f7\u5207\u6362\u7c7b\u522b\uff0cP\u91cd\u590d\u76ee\u7684\u5730\u3002\u53cd\u659c\u6760\u8bfb\u8def\u7ebf\uff0cU\u7b5b\u9009\u65e0\u6cd5\u5230\u8fbe\u7684\u76ee\u7684\u5730\u3002G\u8bfb\u91d1\u94b1\uff0cM\u8bfb\u5730\u56fe\u540d\u3002F6\u5207\u6362\u4fe1\u6807\uff0cF7\u5207\u6362\u81ea\u52a8\u8be6\u60c5\uff0cF8\u6253\u5f00\u6a21\u7ec4\u83dc\u5355\u3002",
            L"Ctrl\u52a0\u65b9\u5411\u952e\u67e5\u770b\u4fa6\u6d4b\u4fe1\u606f\u3002", L"U\u8bfb\u989d\u5916\u4fe1\u606f\u3002", L"\u9017\u53f7\u548c\u53e5\u53f7\u9010\u884c\u67e5\u770b\u3002", L"Shift+I\u6717\u8bfb\u663e\u793a\u7684\u64cd\u4f5c\u952e\u3002"
        },
        {
            L"\ub3c4\uc6c0\ub9d0",
            L"\ubaa8\ub4dc \uba54\ub274. \uc704\uc544\ub798\ub85c \uc120\ud0dd\ud558\uace0 \uc88c\uc6b0\ub85c \ubcc0\uacbd\ud569\ub2c8\ub2e4. Enter \ub610\ub294 A\ub85c \uc2e4\ud589\ud558\uace0 I \ub610\ub294 X\ub85c \uc124\uba85\uc744 \ub4e3\uc2b5\ub2c8\ub2e4. Home\uacfc End\ub294 \ucc98\uc74c\uacfc \ub05d\uc73c\ub85c \uc774\ub3d9\ud569\ub2c8\ub2e4. Esc \ub610\ub294 B\ub294 \ub4a4\ub85c, F8\uc740 \ub2eb\uae30\uc785\ub2c8\ub2e4.",
            L"\uc2a4\ud53c\uc5b4\ubc18 \uacc4\ud68d. \uc704\uc544\ub798\ub85c \ud0d0\uc0c9\ud558\uace0 Page Up\uacfc Page Down\uc73c\ub85c \uac74\ub108\ub701\ub2c8\ub2e4. Enter\ub294 \uc120\ud0dd, I\ub294 \uc124\uba85, Esc\ub294 \ub2eb\uae30\uc785\ub2c8\ub2e4.",
            L"\uc653\uce74\uc758 \ub9b4. \uc704\uc544\ub798\ub85c \uc120\ud0dd\ud558\uace0 Enter \ub610\ub294 A\ub85c \uc801\uc6a9\ud569\ub2c8\ub2e4. I\ub294 \uc124\uba85, Esc \ub610\ub294 B\ub294 \uc218\ub3d9 \ub9b4\ub85c \ub3cc\uc544\uac11\ub2c8\ub2e4.",
            L"\ud14d\uc2a4\ud2b8 \uc785\ub825. F11\ub85c \uc785\ub825\ud55c \ub0b4\uc6a9\uc744 \uc77d\uc2b5\ub2c8\ub2e4.",
            L"\uc804\ud22c. T\ub294 \ud604\uc7ac \ucc28\ub840, H\ub294 HP\uc640 \uc0c1\ud0dc, O\ub294 \uc624\ubc84\ub4dc\ub77c\uc774\ube0c\uc785\ub2c8\ub2e4. Shift+T\ub294 \ud589\ub3d9 \uc21c\uc11c, Shift+H\ub294 \ud45c\uc2dc\ub41c HP\uc640 MP, Shift+O\ub294 \ud45c\uc2dc\ub41c \uac8c\uc774\uc9c0\uc785\ub2c8\ub2e4. I\ub294 \uc0c1\uc138 \uc815\ubcf4, F11\uc740 \ubc18\ubcf5, F8\uc740 \ubaa8\ub4dc \uba54\ub274\uc785\ub2c8\ub2e4.",
            L"\uc2a4\ud53c\uc5b4\ubc18. F2\ub294 \uacbd\ub85c \uacc4\ud68d, F3\uc740 \uc5b4\ube4c\ub9ac\ud2f0 \uacc4\ud68d, Shift+F2\ub294 \uacc4\ud68d \uc9c0\uc6b0\uae30\uc785\ub2c8\ub2e4. F4\uc640 F5\ub294 \uc55e\ub4a4 \uc548\ub0b4\uc785\ub2c8\ub2e4. \uc5ed\uc2ac\ub798\uc2dc\uc640 Shift+\uc5ed\uc2ac\ub798\uc2dc\ub294 \ucee4\uc11c\uc5d0\uc11c \uc548\ub0b4\ud569\ub2c8\ub2e4. O\ub294 \ubc29\ud5a5, S\ub294 \uc2a4\ud53c\uc5b4 \ub808\ubca8, F6\uc740 \ub178\ub4dc \ubc88\ud638 \uc785\ub825\uc785\ub2c8\ub2e4. I\ub294 \uc124\uba85, F7\uc740 \uc790\ub3d9 \uc0c1\uc138 \uc804\ud658, F8\uc740 \ubaa8\ub4dc \uba54\ub274\uc785\ub2c8\ub2e4.",
            L"\uac8c\uc784 \uba54\ub274. I\ub294 \uc0c1\uc138 \uc815\ubcf4, F7\uc740 \uc790\ub3d9 \uc0c1\uc138 \uc804\ud658, F11\uc740 \ubc18\ubcf5, F8\uc740 \ubaa8\ub4dc \uba54\ub274\uc785\ub2c8\ub2e4.",
            L"\uc2dc\ub828\uc758 \ubc29. F9\uc640 F10\uc740 \ub2e8\uacc4 \uc774\ub3d9, Shift+F10\uc740 \uc120\ud0dd\ud55c \ub2e8\uacc4, F11\uc740 \ud604\uc7ac \ub2e8\uacc4, Shift+F11\uc740 \ucd94\uc815 \ub2e8\uacc4\uc785\ub2c8\ub2e4. F8, Miscellaneous, Cloister of Trials.",
            L"\uc815\ud654\uc758 \uae38. F9\uc640 F10\uc740 \ub2e8\uacc4 \uc774\ub3d9, Shift+F10\uc740 \uc120\ud0dd\ud55c \ub2e8\uacc4, F11\uc740 \ud604\uc7ac \ub2e8\uacc4, Shift+F11\uc740 \ucd94\uc815 \ub2e8\uacc4\uc785\ub2c8\ub2e4. F8, Miscellaneous, Via Purifico.",
            L"\uac00\uac00\uc81c\ud2b8 \ub3d9\uad74 \ud37c\uc990. F11\uc740 \uc124\uba85\uc744 \uc77d\uace0 \ud37c\uc990 \uc911 Shift+F11\uc740 \uc120\ud0dd\ud55c \uc785\uad6c\ub97c \uc77d\uc2b5\ub2c8\ub2e4. F8, Miscellaneous, Gagazet cave puzzles.",
            L"\ud544\ub4dc. \ubc31\ud2f1\uc740 \ub2e4\uc2dc \uac80\uc0c9, \ub300\uad04\ud638\ub294 \ubaa9\uc801\uc9c0 \uc120\ud0dd, Shift\uc640 \ub300\uad04\ud638\ub294 \ubd84\ub958 \ubcc0\uacbd, P\ub294 \ubaa9\uc801\uc9c0 \ubc18\ubcf5\uc785\ub2c8\ub2e4. \uc5ed\uc2ac\ub798\uc2dc\ub294 \uae38 \uc548\ub0b4, U\ub294 \ub3c4\ub2ec\ud560 \uc218 \uc5c6\ub294 \ubaa9\uc801\uc9c0 \ud544\ud130, G\ub294 \uae38, M\uc740 \uc9c0\ub3c4 \uc774\ub984\uc785\ub2c8\ub2e4. F6\uc740 \uc2e0\ud638 \uc804\ud658, F7\uc740 \uc790\ub3d9 \uc0c1\uc138 \uc804\ud658, F8\uc740 \ubaa8\ub4dc \uba54\ub274\uc785\ub2c8\ub2e4.",
            L"Ctrl\uacfc \ubc29\ud5a5\ud0a4\ub85c \uc13c\uc11c \uc815\ubcf4\ub97c \ud655\uc778\ud569\ub2c8\ub2e4.", L"U\ub85c \ucd94\uac00 \uc815\ubcf4\ub97c \uc77d\uc2b5\ub2c8\ub2e4.", L"\uc27c\ud45c\uc640 \ub9c8\uce68\ud45c\ub85c \uc904\uc744 \ud0d0\uc0c9\ud569\ub2c8\ub2e4.", L"Shift+I\ub85c \ud45c\uc2dc\ub41c \uc870\uc791\ubc95\uc744 \uc77d\uc2b5\ub2c8\ub2e4."
        }
    };
    return catalog[menuLanguageIndex(locale)];
}
}
