    #include <poppler/cpp/poppler-document.h>
    #include <poppler/cpp/poppler-page.h>
    #include <windows.h>
    #include <shobjidl.h>
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

    double calculate_total_length(const vector<fs::path>& pdf_files, double threshold_mm) {
        double total = 0.0;

        for (const auto& pdf_path : pdf_files) {
            auto doc = unique_ptr<poppler::document>(
                poppler::document::load_from_file(pdf_path.string())
            );
            if (!doc) continue;

            int n_pages = doc->pages();

            for (int i = 0; i < n_pages; ++i) {
                auto page = unique_ptr<poppler::page>(doc->create_page(i));
                if (!page) continue;

                poppler::rectf rect = page->page_rect();
                double w_mm = rect.width()  * PT_TO_MM;
                double h_mm = rect.height() * PT_TO_MM;

                double longer  = max(w_mm, h_mm);
                double shorter = min(w_mm, h_mm);

                if (w_mm > threshold_mm || h_mm > threshold_mm)
                    total += longer;
                else
                    total += shorter;
            }
        }

        return total;
    }

    void extract_text(const fs::path& pdf_path, ostream& out) {
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

        for (int i = 0; i < n_pages; ++i) {
        auto page = unique_ptr<poppler::page>(doc->create_page(i));
        if (!page) continue;

        poppler::ustring text = page->text();
        out << text.to_latin1() << "\n";
        }
    }

    bool promptYesNo(string text) {
        bool YesNo = false;
        string value;

        do {
            cout << text << endl;
            cout << "Y si, N no:" << endl;
            cin >> value;

            // Conversion to lowercase
            for (int i=0; i < value.length(); i++) value[i] = tolower(value[i]);

            // Check values
            if  (value == "yes") value = "y";
            if  (value == "no") value = "n";
            if  (value == "y") YesNo = true;
            if  (value == "n") YesNo = false;
        }   while (value != "y" && value != "n");
        return YesNo;
    }

    vector<fs::path> open_file_dialog() {
        vector<fs::path> selected_files;

        CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        IFileOpenDialog* pFileOpen = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                                    IID_IFileOpenDialog, (void**)&pFileOpen);

        if (SUCCEEDED(hr)) {
            DWORD dwOptions;
            pFileOpen->GetOptions(&dwOptions);
            pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);

            COMDLG_FILTERSPEC filter[] = {
                { L"PDF Files", L"*.pdf" },
                { L"All Files", L"*.*" }
            };
            pFileOpen->SetFileTypes(2, filter);

            hr = pFileOpen->Show(NULL);

            if (SUCCEEDED(hr)) {
                IShellItemArray* pItems = nullptr;
                hr = pFileOpen->GetResults(&pItems);

                if (SUCCEEDED(hr)) {
                    DWORD count;
                    pItems->GetCount(&count);

                    for (DWORD i = 0; i < count; i++) {
                        IShellItem* pItem = nullptr;
                        pItems->GetItemAt(i, &pItem);

                        PWSTR pszFilePath = nullptr;
                        pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        selected_files.push_back(fs::path(pszFilePath));

                        CoTaskMemFree(pszFilePath);
                        pItem->Release();
                    }
                    pItems->Release();
                }
            }
            pFileOpen->Release();
        }

        CoUninitialize();
        return selected_files;
    }


    int main(int argc, char* argv[]) {
        // Risale di un livello rispetto all'exe per trovare input e output
        fs::path exe_dir    = fs::path(argv[0]).parent_path();
        fs::path base_dir   = exe_dir.parent_path();
        fs::path input_dir  = base_dir / "input";
        fs::path output_dir = base_dir / "output";
        int choice;
        bool choicedone = true;

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

        while (choicedone != false){
            cout << "*******************************\n";
            cout << " 1 - Seleziona i PDF per l'operazione. \n";
            cout << " 2 - Visualizza PDF.\n";
            cout << " 3 - Misura lunghezza totale.\n";
            cout << " 4 - Estrapola il testo di un pdf.\n";
            cout << " 5 - Esci.\n";
            cout << " Seleziona opzione e premi invio: ";

            cin >> choice;


    switch (choice)
    {
    case 1:{
        vector<fs::path> selected = open_file_dialog();
            if (selected.empty()) {
                cout << "Nessun file selezionato.\n";
                }
            else {
                pdf_files = selected;
                cout << pdf_files.size() << " file selezionati.\n";
                for (const auto& p : pdf_files)
                cout << "  " << p.filename().string() << "\n";    
            }       
        break;
    }
    case 2:
        for (const auto& path : pdf_files) {
            analyze_pdf(path, cout);
            analyze_pdf(path, out);
        }
        cout << "\nRisultati salvati in: " << output_file.string() << "\n";
        break;

    case 3:
    {
        double threshold = 0.0;
        int t_choice = 0;
        float price = 0;
        float m_lenght = 0;

        cout << "\nSeleziona la soglia:\n";
        cout << " 1 - 900mm\n";
        cout << " 2 - 920mm\n";
        cout << " 3 - 1040mm\n";
        cout << " Seleziona: ";
        cin >> t_choice;

        switch (t_choice) {
            case 1: threshold = 900.0;  break;
            case 2: threshold = 920.0;  break;
            case 3: threshold = 1040.0; break;
            default:
                cout << "Selezione non valida, uso 900mm come default.\n";
                threshold = 900.0;
                break;
        }

        double total = calculate_total_length(pdf_files, threshold);
        cout << fixed << setprecision(1);
        cout << "\nSoglia: " << threshold << "mm\n";
        cout << "Lunghezza totale: " << total << "mm ("
            << total / 10.0 << "cm)\n";
        bool status = promptYesNo("Vuoi calcolare anche il costo al metro di questo ordine?\n");

        if (status){
            cout << "Inserire il prezzo al metro della carta.\n";
            cin >> price;
            m_lenght = total / 1000;
            price = price * m_lenght;
            cout << "il prezzo totale della stampa di questi file e' di:" << price << " Euro.\n";
            break;
        }
        else
        break;
    }                
    case 4:
        for (const auto& path : pdf_files) {
            extract_text(path, cout);
            extract_text(path, out);
        }
        cout << "\nRisultati salvati in: " << output_file.string() << "\n";
        break;
    case 5:
        return 1;

    default:
        cout << "Selezione invalida. Riprova.\n";
        break;
    }
        }
    }