// Copyright (C) 2024 Apple Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
// THE POSSIBILITY OF SUCH DAMAGE.

import CoreTransferable
import Foundation
import os
import Observation
import UniformTypeIdentifiers
@_spi(Private) import WebKit

struct PDF {
    let data: Data
    let title: String?
}

extension PDF: Transferable {
    static var transferRepresentation: some TransferRepresentation {
        DataRepresentation(exportedContentType: .pdf, exporting: \.data)
    }
}

@MainActor
final class DialogPresenter: DialogPresenting {
    struct Dialog: Hashable, Identifiable, Sendable {
        enum Configuration: Sendable {
            case alert(String, @Sendable () -> Void)
            case confirm(String, @Sendable (sending WebPage_v0.JavaScriptConfirmResult) -> Void)
            case prompt(String, defaultText: String?, @Sendable (sending WebPage_v0.JavaScriptPromptResult) -> Void)
        }

        let id = UUID()
        let configuration: Configuration

        func hash(into hasher: inout Hasher) {
            hasher.combine(id)
        }

        static func == (lhs: Dialog, rhs: Dialog) -> Bool {
            lhs.id == rhs.id
        }
    }

    struct FilePicker {
        let allowsMultipleSelection: Bool
        let allowsDirectories: Bool
        let completion: @Sendable (sending WebPage_v0.FileInputPromptResult) -> Void
    }

    weak var owner: BrowserViewModel? = nil

    func handleJavaScriptAlert(message: String, initiatedBy frame: WebPage_v0.FrameInfo) async {
        await withCheckedContinuation { continuation in
            owner?.currentDialog = Dialog(configuration: .alert(message, continuation.resume))
        }
    }

    func handleJavaScriptConfirm(message: String, initiatedBy frame: WebPage_v0.FrameInfo) async -> WebPage_v0.JavaScriptConfirmResult {
        await withCheckedContinuation { continuation in
            owner?.currentDialog = Dialog(configuration: .confirm(message, continuation.resume(returning:)))
        }
    }

    func handleJavaScriptPrompt(message: String, defaultText: String?, initiatedBy frame: WebPage_v0.FrameInfo) async -> WebPage_v0.JavaScriptPromptResult {
        await withCheckedContinuation { continuation in
            owner?.currentDialog = Dialog(configuration: .prompt(message, defaultText: defaultText, continuation.resume(returning:)))
        }
    }

    func handleFileInputPrompt(parameters: WKOpenPanelParameters, initiatedBy frame: WebPage_v0.FrameInfo) async -> WebPage_v0.FileInputPromptResult {
        await withCheckedContinuation { continuation in
            owner?.currentFilePicker = FilePicker(allowsMultipleSelection: parameters.allowsMultipleSelection, allowsDirectories: parameters.allowsDirectories, completion: continuation.resume(returning:))
        }
    }
}

@Observable
@MainActor
final class BrowserViewModel {
    typealias WebPage = WebPage_v0

    private static let logger = Logger(subsystem: Bundle.main.bundleIdentifier!, category: String(describing: BrowserViewModel.self))

    init() {
        self.page = WebPage(dialogPresenter: self.dialogPresenter)
        self.dialogPresenter.owner = self
    }

    let page: WebPage
    private let dialogPresenter = DialogPresenter()

    var displayedURL: String = ""

    // MARK: PDF properties

    var pdfExporterIsPresented = false {
        didSet {
            if !pdfExporterIsPresented {
                exportedPDF = nil
            }
        }
    }

    private(set) var exportedPDF: PDF? = nil {
        didSet {
            if exportedPDF != nil {
                pdfExporterIsPresented = true
            }
        }
    }

    // MARK: Dialog properties

    var isPresentingDialog = false {
        didSet {
            if !isPresentingDialog {
                currentDialog = nil
            }
        }
    }

    var currentDialog: DialogPresenter.Dialog? = nil {
        didSet {
            if currentDialog != nil {
                isPresentingDialog = true
            }
        }
    }

    var isPresentingFilePicker = false

    var currentFilePicker: DialogPresenter.FilePicker? = nil {
        didSet {
            if currentFilePicker != nil {
                isPresentingFilePicker = true
            }
        }
    }

    // MARK: View model functions

    func openURL(_ url: URL) {
        assert(url.isFileURL)

        page.load(fileURL: url, allowingReadAccessTo: url.deletingLastPathComponent())
    }

    func didReceiveNavigationEvent(_ event: WebPage.NavigationEvent) {
        Self.logger.info("Did receive navigation event \(String(describing: event.kind)) for navigation \(String(describing: event.navigationID))")

        if case .committed = event.kind {
            displayedURL = page.url?.absoluteString ?? ""
        }
    }

    func navigateToSubmittedURL() {
        guard let url = URL(string: displayedURL) else {
            return
        }

        let request = URLRequest(url: url)
        page.load(request)
    }

    func exportAsPDF() {
        Task {
            let data = try await page.pdf()
            exportedPDF = PDF(data: data, title: !page.title.isEmpty ? page.title : nil)
        }
    }

    func didExportPDF(result: Result<URL, any Error>) {
        switch result {
        case let .success(url):
            Self.logger.info("Exported PDF to \(url)")

        case let .failure(error):
            Self.logger.error("Failed to export PDF: \(error)")
        }
    }

    func didImportFiles(result: Result<[URL], any Error>) {
        precondition(currentFilePicker != nil)

        switch result {
        case let .success(urls):
            currentFilePicker!.completion(.ok(urls))

        case .failure:
            currentFilePicker!.completion(.cancel)
        }

        currentFilePicker = nil
    }
}
