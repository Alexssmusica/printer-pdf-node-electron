#ifndef PRINTER_LINUX
#define PRINTER_LINUX
#include "inc.h"
#include "printer_interface.h"
#include <cups/cups.h>

namespace printer_pdf_electron_node {

// Implementation of the forward-declared LinuxPrinter
class LinuxPrinter : public PrinterInterface {
public:
    LinuxPrinter() = default;
    ~LinuxPrinter();
    std::string Initialize(const Napi::Value& printerName) override;
    bool Print(const std::string& filePath, const PdfiumOption& options) override;

private:
    std::string printer_name;
    cups_dest_t* printer_dest = nullptr;
    int num_options = 0;
    cups_option_t* options = nullptr;
    void CleanupOptions();
};

} // namespace printer_pdf_electron_node

#endif 