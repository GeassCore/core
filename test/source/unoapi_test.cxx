/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/unoapi_test.hxx>

#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>

#include <sfx2/app.hxx>
#include <sfx2/objsh.hxx>
#include <unotools/mediadescriptor.hxx>
#include <utility>

using namespace css;
using namespace css::uno;

UnoApiTest::UnoApiTest(OUString path)
    : mbSkipValidation(false)
    , m_aBaseString(std::move(path))
{
    maTempFile.EnableKillingFile();
}

void UnoApiTest::setUp()
{
    test::BootstrapFixture::setUp();

    mxDesktop
        = css::frame::Desktop::create(comphelper::getComponentContext(getMultiServiceFactory()));
    CPPUNIT_ASSERT_MESSAGE("no desktop!", mxDesktop.is());
    SfxApplication::GetOrCreate();
}

void UnoApiTest::tearDown()
{
    if (mxComponent.is())
    {
        mxComponent->dispose();
        mxComponent.clear();
    }

    test::BootstrapFixture::tearDown();
}

OUString UnoApiTest::createFileURL(std::u16string_view aFileBase)
{
    return m_directories.getSrcRootURL() + m_aBaseString + "/" + aFileBase;
}

OUString UnoApiTest::loadFromURL(std::u16string_view aFileBase)
{
    if (mxComponent.is())
    {
        mxComponent->dispose();
        mxComponent.clear();
    }

    OUString aFileName = createFileURL(aFileBase);
    mxComponent = loadFromDesktop(aFileName);
    return aFileName;
}

uno::Any UnoApiTest::executeMacro(const OUString& rScriptURL,
                                  const uno::Sequence<uno::Any>& rParams)
{
    uno::Any aRet;
    uno::Sequence<sal_Int16> aOutParamIndex;
    uno::Sequence<uno::Any> aOutParam;

    ErrCode result = SfxObjectShell::CallXScript(mxComponent, rScriptURL, rParams, aRet,
                                                 aOutParamIndex, aOutParam);
    CPPUNIT_ASSERT_EQUAL(ERRCODE_NONE, result);

    return aRet;
}

void UnoApiTest::save(const OUString& rFilter, const char* pPassword)
{
    utl::MediaDescriptor aMediaDescriptor;
    aMediaDescriptor["FilterName"] <<= rFilter;
    if (!maFilterOptions.isEmpty())
        aMediaDescriptor["FilterOptions"] <<= maFilterOptions;
    css::uno::Reference<frame::XStorable> xStorable(mxComponent, css::uno::UNO_QUERY_THROW);

    if (pPassword)
    {
        if (rFilter != "Office Open XML Text")
        {
            aMediaDescriptor["Password"] <<= OUString::createFromAscii(pPassword);
        }
        else
        {
            OUString sPassword = OUString::createFromAscii(pPassword);
            uno::Sequence<beans::NamedValue> aEncryptionData{
                { "CryptoType", uno::Any(OUString("Standard")) },
                { "OOXPassword", uno::Any(sPassword) }
            };
            aMediaDescriptor[utl::MediaDescriptor::PROP_ENCRYPTIONDATA] <<= aEncryptionData;
        }
    }

    xStorable->storeToURL(maTempFile.GetURL(), aMediaDescriptor.getAsConstPropertyValueList());

    if (!mbSkipValidation)
    {
        if (rFilter == "Office Open XML Text")
            validate(maTempFile.GetFileName(), test::OOXML);
        else if (rFilter == "Calc Office Open XML")
            validate(maTempFile.GetFileName(), test::OOXML);
        else if (rFilter == "Impress Office Open XML")
            validate(maTempFile.GetFileName(), test::OOXML);
        else if (rFilter == "writer8")
            validate(maTempFile.GetFileName(), test::ODF);
        else if (rFilter == "calc8")
            validate(maTempFile.GetFileName(), test::ODF);
        else if (rFilter == "impress8")
            validate(maTempFile.GetFileName(), test::ODF);
        else if (rFilter == "draw8")
            validate(maTempFile.GetFileName(), test::ODF);
        else if (rFilter == "OpenDocument Text Flat XML")
            validate(maTempFile.GetFileName(), test::ODF);
        else if (rFilter == "MS Word 97")
            validate(maTempFile.GetFileName(), test::MSBINARY);
        else if (rFilter == "MS Excel 97")
            validate(maTempFile.GetFileName(), test::MSBINARY);
        else if (rFilter == "MS PowerPoint 97")
            validate(maTempFile.GetFileName(), test::MSBINARY);
    }
}

void UnoApiTest::saveAndClose(const OUString& rFilter)
{
    save(rFilter);

    mxComponent->dispose();
    mxComponent.clear();
}

void UnoApiTest::saveAndReload(const OUString& rFilter)
{
    saveAndClose(rFilter);

    mxComponent = loadFromDesktop(maTempFile.GetURL());
}

std::unique_ptr<vcl::pdf::PDFiumDocument> UnoApiTest::parsePDFExport(const OString& rPassword)
{
    SvFileStream aFile(maTempFile.GetURL(), StreamMode::READ);
    maMemory.WriteStream(aFile);
    std::shared_ptr<vcl::pdf::PDFium> pPDFium = vcl::pdf::PDFiumLibrary::get();
    if (!pPDFium)
    {
        return nullptr;
    }
    std::unique_ptr<vcl::pdf::PDFiumDocument> pPdfDocument
        = pPDFium->openDocument(maMemory.GetData(), maMemory.GetSize(), rPassword);
    CPPUNIT_ASSERT(pPdfDocument);
    return pPdfDocument;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
