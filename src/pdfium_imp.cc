#include "pdfium_imp.h"
#include <clocale>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <windows.h>
#include <winspool.h>
using namespace std;
namespace printer_pdf_electron_node
{

#ifdef _WIN32

    PrinterDocumentJob::PrinterDocumentJob(DeviceContext dc, const std::wstring &filename)
        : dc_(dc), filename_(filename), jobId_(0), cancelled_(false)
    {
        try
        {
            LogError("Starting print job for: " + std::string(filename_.begin(), filename_.end()));

            DOCINFOW di = {0};
            di.cbSize = sizeof(DOCINFOW);
            di.lpszDocName = filename_.c_str();
            di.lpszOutput = nullptr;
            di.lpszDatatype = nullptr;
            di.fwType = 0;

            jobId_ = ::StartDocW(dc_, &di);
            if (jobId_ <= 0)
            {
                cancelled_ = true;
                DWORD error = GetLastError();
                std::string errorMsg = "StartDocW failed with error: " + std::to_string(error);
                LogError(errorMsg);
                throw std::runtime_error(errorMsg);
            }

            LogError("Print job started successfully");
        }
        catch (const std::exception &e)
        {
            LogError("Exception in PrinterDocumentJob constructor: " + std::string(e.what()));
            throw;
        }
        catch (...)
        {
            LogError("Unknown exception in PrinterDocumentJob constructor");
            throw;
        }
    }

    PrinterDocumentJob::~PrinterDocumentJob()
    {
        try
        {
            if (jobId_ > 0)
            {
                if (::EndDoc(dc_) <= 0)
                {
                    DWORD error = GetLastError();
                    std::cerr << "EndDoc failed with error: " << error << std::endl;
                }
            }
        }
        catch (...)
        {
            // Nunca deixar exceções escaparem do destrutor
            std::cerr << "Unexpected error in PrinterDocumentJob destructor" << std::endl;
        }
    }

    bool PrinterDocumentJob::Start()
    {
        return jobId_ > 0;
    }

    bool PrinterDocumentJob::IsCancelled() const
    {
        return cancelled_;
    }

    PDFDocument::PDFDocument(std::wstring &&f) : filename(f)
    {
        LogError("Creating PDFDocument for file: " + std::string(filename.begin(), filename.end()));
    }

    bool PDFDocument::LoadDocument()
    {
        try
        {
            LogError("Loading document: " + std::string(filename.begin(), filename.end()));

            std::ifstream pdfStream(std::string(filename.begin(), filename.end()),
                                    std::ifstream::binary | std::ifstream::in);

            if (!pdfStream.is_open())
            {
                LogError("Failed to open PDF file");
                return false;
            }

            file_content.insert(file_content.end(),
                                std::istreambuf_iterator<char>(pdfStream),
                                std::istreambuf_iterator<char>());

            if (file_content.empty())
            {
                LogError("PDF file is empty");
                return false;
            }

            auto pdf_pointer = FPDF_LoadMemDocument(file_content.data(), (int)file_content.size(), nullptr);
            if (!pdf_pointer)
            {
                DWORD error = FPDF_GetLastError();
                LogError("Failed to load PDF document. Error: " + std::to_string(error));
                return false;
            }

            doc.reset(pdf_pointer);
            LogError("Document loaded successfully");
            return true;
        }
        catch (const std::exception &e)
        {
            LogError("Exception in LoadDocument: " + std::string(e.what()));
            return false;
        }
        catch (...)
        {
            LogError("Unknown exception in LoadDocument");
            return false;
        }
    }

    void PDFDocument::PrintDocument(DeviceContext dc, const PdfiumOption &options)
    {
        try
        {
            PrinterDocumentJob djob(dc, filename);
            if (!djob.Start())
            {
                DWORD error = GetLastError();
                std::string errorMsg = "Failed to start print job. Error: " + std::to_string(error);
                LogError(errorMsg);
                throw std::runtime_error(errorMsg);
            }

            auto pageCount = FPDF_GetPageCount(doc.get());
            if (pageCount <= 0)
            {
                std::string errorMsg = "Invalid page count: " + std::to_string(pageCount);
                LogError(errorMsg);
                throw std::runtime_error(errorMsg);
            }

            auto width = options.width;
            auto height = options.height;

            for (int16_t i = 0; i < options.copies; ++i)
            {
                if (std::size(options.page_list) > 0)
                {
                    for (auto pair : options.page_list)
                    {
                        for (auto j = pair.first; j < pair.second + 1; ++j)
                        {
                            if (djob.IsCancelled())
                            {
                                return;
                            }
                            printPage(dc, j, width, height, options.dpi, options);
                        }
                    }
                }
                else
                {
                    for (auto j = 0; j < pageCount; j++)
                    {
                        if (djob.IsCancelled())
                        {
                            return;
                        }
                        printPage(dc, j, width, height, options.dpi, options);
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            LogError(std::string("PrintDocument error: ") + e.what());
            throw; // Propaga a exceção para ser tratada em nível superior
        }
        catch (...)
        {
            LogError("Unknown error in PrintDocument");
            throw; // Propaga a exceção para ser tratada em nível superior
        }
    }

    void PDFDocument::printPage(DeviceContext dc, int32_t index, int32_t width, int32_t height,
                                float dpiRatio, const PdfiumOption &options)
    {
        try
        {
            PrinterPageJob pJob(dc);

            FPDF_PAGE page = nullptr;
            try
            {
                page = getPage(doc.get(), index);
                if (!page)
                {
                    throw std::runtime_error("Failed to get page " + std::to_string(index));
                }

                // Garantir que estamos no modo correto
                if (!SetGraphicsMode(dc, GM_ADVANCED))
                {
                    throw std::runtime_error("SetGraphicsMode failed: " + std::to_string(GetLastError()));
                }
                if (!SetMapMode(dc, MM_TEXT))
                {
                    throw std::runtime_error("SetMapMode failed: " + std::to_string(GetLastError()));
                }
                if (!SetBkMode(dc, TRANSPARENT))
                {
                    throw std::runtime_error("SetBkMode failed: " + std::to_string(GetLastError()));
                }

                // Get physical page dimensions
                int physicalWidth = GetDeviceCaps(dc, PHYSICALWIDTH);
                int physicalHeight = GetDeviceCaps(dc, PHYSICALHEIGHT);
                int physicalOffsetX = GetDeviceCaps(dc, PHYSICALOFFSETX);
                int physicalOffsetY = GetDeviceCaps(dc, PHYSICALOFFSETY);

                // Calculate printable area
                int printableWidth = GetDeviceCaps(dc, HORZRES);
                int printableHeight = GetDeviceCaps(dc, VERTRES);

                // Apply margins to printable area (converting from points to device units)
                int marginLeft = static_cast<int>((options.margins.left / 72.0f) * dpiRatio);
                int marginTop = static_cast<int>((options.margins.top / 72.0f) * dpiRatio);
                int marginRight = static_cast<int>((options.margins.right / 72.0f) * dpiRatio);
                int marginBottom = static_cast<int>((options.margins.bottom / 72.0f) * dpiRatio);

                // Adjust printable area considering margins
                int effectivePrintableWidth = printableWidth - (marginLeft + marginRight);
                int effectivePrintableHeight = printableHeight - (marginTop + marginBottom);

                // If no width/height specified, get from PDF page
                if (!width)
                {
                    auto pageWidth = FPDF_GetPageWidth(page);
                    width = static_cast<int32_t>(pageWidth * dpiRatio);
                }
                if (!height)
                {
                    auto pageHeight = FPDF_GetPageHeight(page);
                    height = static_cast<int32_t>(pageHeight * dpiRatio);
                }

                // Create clipping region for the printable area (including margins)
                HRGN rgn = CreateRectRgn(0, 0, printableWidth, printableHeight);
                SelectClipRgn(dc, rgn);
                DeleteObject(rgn);

                // Clear the background
                Rectangle(dc, -physicalOffsetX, -physicalOffsetY, physicalWidth, physicalHeight);

                float scale;
                int32_t x, y;
                if (options.fitToPage)
                {
                    // Calculate scaling to fit page while maintaining aspect ratio
                    float scaleX = static_cast<float>(effectivePrintableWidth) / width;
                    float scaleY = static_cast<float>(effectivePrintableHeight) / height;
                    scale = scaleX < scaleY ? scaleX : scaleY;

                    // Calculate centered position within the effective printable area
                    x = marginLeft + (effectivePrintableWidth - static_cast<int32_t>(width * scale)) / 2;
                    y = marginTop + (effectivePrintableHeight - static_cast<int32_t>(height * scale)) / 2;
                }
                else
                {
                    // Use actual size (1:1 scale)
                    scale = 1.0f;

                    // Center the content in the available space
                    x = marginLeft + (effectivePrintableWidth - width) / 2;
                    y = marginTop + (effectivePrintableHeight - height) / 2;

                    // If content is larger than printable area, align to top-left corner
                    if (width > effectivePrintableWidth || height > effectivePrintableHeight)
                    {
                        x = marginLeft;
                        y = marginTop;
                    }
                }

                // Render the PDF page
                ::FPDF_RenderPage(dc, page, x, y,
                                  static_cast<int32_t>(width * scale),
                                  static_cast<int32_t>(height * scale),
                                  0, FPDF_ANNOT | FPDF_PRINTING);

                // Garantir que todas as operações GDI foram concluídas
                if (!GdiFlush())
                {
                    throw std::runtime_error("GdiFlush failed: " + std::to_string(GetLastError()));
                }
            }
            catch (...)
            {
                // Propaga a exceção mas garante limpeza adequada
                throw;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error printing page " << index << ": " << e.what() << std::endl;
            throw; // Propaga para tratamento superior
        }
        catch (...)
        {
            std::cerr << "Unknown error printing page " << index << std::endl;
            throw; // Propaga para tratamento superior
        }
    }

    FPDF_PAGE PDFDocument::getPage(const FPDF_DOCUMENT &doc, int32_t index)
    {
        if (!doc)
        {
            throw std::runtime_error("Invalid document handle");
        }

        auto iter = loaded_pages.find(index);
        if (iter != loaded_pages.end())
            return iter->second.get();

        ScopedFPDFPage page(FPDF_LoadPage(doc, index));
        if (!page)
        {
            throw std::runtime_error("Failed to load page " + std::to_string(index));
        }

        FPDF_PAGE page_ptr = page.get();
        loaded_pages[index] = std::move(page);
        return page_ptr;
    }

#endif

}
