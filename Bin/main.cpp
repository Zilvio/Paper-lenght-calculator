/**
 * main.cpp
 * Author: stdf
 *
 * Legge tutti i PDF dalla cartella "input" e scrive le dimensioni
 * di ogni pagina in un file di testo nella cartella "output".
 *
 * Struttura attesa:
 *   MisuraTavole/
 *       main.exe
 *       input/      <-- metti qui i PDF
 *       output/     <-- il risultato viene scritto qui
 *
 * Compilazione (MSYS2 MinGW x64):
 *   g++ -std=c++17 main.cpp -o main.exe $(pkg-config --cflags --libs poppler-cpp)
 */

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>

using namespace std;
namespace fs = std::filesystem;

// Poppler usa i "punti PDF" (1 pt = 1/72 pollice)
constexpr double PT_TO_MM = 25.4 / 72.0;
constexpr double PT_TO_CM = 2.54 / 72.0;

// Formati standard (± 1.5 mm di tolleranza)
struct PaperFormat {
    const char* name;
    double width_mm;
    double height_mm;
};

static const PaperFormat PAPER_FORMATS[] = {
    {"A0",      841.0,  1189.0},
    {"A1",      594.0,   841.0},
    {"A2",      420.0,   594.0},
    {"A3",      297.0,   420.0},
    {"A4",      210.0,   297.0},
    {"A5",      148.0,   210.0},
    {"A6",      105.0,   148.0},
    {"B4",      250.0,   353.0},
    {"B5",      176.0,   250.0},
    {"Letter",  215.9,   279.4},
    {"Legal",   215.9,   355.6},
    {"Tabloid", 279.4,   431.8},
};

string detect_format(double w_mm, double h_mm) {
    double short_side = min(w_mm, h_mm);
    double long_side  = max(w_mm, h_mm);

    for (const auto& fmt : PAPER_FORMATS) {
        double fs = min(fmt.width_mm, fmt.height_mm);
        double fl = max(fmt.width_mm, fmt.height_mm);
        if (abs(short_side - fs) <= 1.5 && abs(long_side - fl) <= 1.5)
            return string(fmt.name) + (w_mm > h_mm ? " landscape" : " portrait");
    }
    return "custom";
}

void print_separator(ostream& out, char c, int n) {
    out << string(n, c) << "\n";
}

void analyze_pdf(const fs::path& pdf_path, ostream& out) {
    auto doc = unique_ptr<poppler::document>(
        poppler::document::load_from_file(pdf_path.string())
    );

    if (!doc) {
        cerr << "  [ERRORE] Impossibile aprire: " << pdf_path.filename() << "\n";
        return;
    }

    int n_pages = doc->pages();

    print_separator(out, '-', 72);
    out << "File  : " << pdf_path.filename().string() << "\n";
    out << "Pagine: " << n_pages << "\n";
    print_separator(out, '-', 72);

    out << left
        << setw(6)  << "Pag."
        << setw(20) << "Larghezza"
        << setw(20) << "Altezza"
        << setw(20) << "Formato"
        << "\n";
    print_separator(out, '-', 72);

    for (int i = 0; i < n_pages; ++i) {
        auto page = unique_ptr<poppler::page>(doc->create_page(i));
        if (!page) continue;

        poppler::rectf rect = page->page_rect();
        double w_mm = rect.width()  * PT_TO_MM;
        double h_mm = rect.height() * PT_TO_MM;
        double w_cm = rect.width()  * PT_TO_CM;
        double h_cm = rect.height() * PT_TO_CM;

        string fmt = detect_format(w_mm, h_mm);

        ostringstream ws, hs;
        ws << fixed << setprecision(1) << w_mm << "mm (" << w_cm << "cm)";
        hs << fixed << setprecision(1) << h_mm << "mm (" << h_cm << "cm)";

        out << fixed << setprecision(1)
            << left << setw(6)  << (i + 1)
            << left << setw(20) << ws.str()
            << left << setw(20) << hs.str()
            << fmt << "\n";
    }

    out << "\n";
}

int main(int argc, char* argv[]) {
    // Risale di un livello rispetto all'exe per trovare input e output
    fs::path exe_dir    = fs::path(argv[0]).parent_path();
    fs::path base_dir   = exe_dir.parent_path();
    fs::path input_dir  = base_dir / "input";
    fs::path output_dir = base_dir / "output";

    // Verifica cartella input
    if (!fs::exists(input_dir) || !fs::is_directory(input_dir)) {
        cerr << "Errore: cartella 'input' non trovata in " << base_dir << "\n";
        system("pause");
        return 1;
    }

    // Crea cartella output se non esiste
    if (!fs::exists(output_dir))
        fs::create_directory(output_dir);

    // Raccoglie tutti i PDF nella cartella input
    vector<fs::path> pdf_files;
    for (const auto& entry : fs::recursive_directory_iterator(input_dir)) {
        if (entry.is_regular_file()) {
            string ext = entry.path().extension().string();
            transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".pdf")
                pdf_files.push_back(entry.path());
        }
    }

    sort(pdf_files.begin(), pdf_files.end());

    // File di output
    fs::path output_file = output_dir / "risultati.txt";
    ofstream out(output_file);
    if (!out.is_open()) {
        cerr << "Errore: impossibile creare il file di output.\n";
        system("pause");
        return 1;
    }

    // Intestazione
    print_separator(cout, '=', 72);
    print_separator(out,  '=', 72);
    cout << "  PDF Page Size Analyzer - stdf\n";
    out  << "  PDF Page Size Analyzer - stdf\n";
    cout << "  Input : " << input_dir.string()  << "\n";
    out  << "  Input : " << input_dir.string()  << "\n";
    cout << "  Output: " << output_file.string() << "\n";
    out  << "  Output: " << output_file.string() << "\n";
    cout << "  PDF trovati: " << pdf_files.size() << "\n";
    out  << "  PDF trovati: " << pdf_files.size() << "\n";
    print_separator(cout, '=', 72);
    print_separator(out,  '=', 72);
    cout << "\n";
    out  << "\n";

    if (pdf_files.empty()) {
        cout << "Nessun file PDF trovato nella cartella input.\n";
        out  << "Nessun file PDF trovato nella cartella input.\n";
        system("pause");
        return 0;
    }

    // Analizza ogni PDF scrivendo sia su console che su file
    for (const auto& path : pdf_files) {
        analyze_pdf(path, cout);
        analyze_pdf(path, out);
    }

    cout << "\nRisultati salvati in: " << output_file.string() << "\n";
    system("pause");
    return 0;
}