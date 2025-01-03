/*
 * Copyright (C) 2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE(WEB_TRANSPORT)

#import "config.h"

#import "HTTPServer.h"
#import "PlatformUtilities.h"
#import "Test.h"
#import "TestNavigationDelegate.h"
#import "TestUIDelegate.h"
#import "Utilities.h"
#import "WebTransportServer.h"
#import <WebKit/WKPreferencesPrivate.h>
#import <WebKit/_WKInternalDebugFeature.h>

namespace TestWebKitAPI {

static void enableWebTransport(WKWebViewConfiguration *configuration)
{
    auto preferences = [configuration preferences];
    for (_WKFeature *feature in [WKPreferences _features]) {
        if ([feature.key isEqualToString:@"WebTransportEnabled"]) {
            [preferences _setEnabled:YES forFeature:feature];
            break;
        }
    }
}

static void validateChallenge(NSURLAuthenticationChallenge *challenge, uint16_t port)
{
    EXPECT_WK_STREQ(challenge.protectionSpace.authenticationMethod, NSURLAuthenticationMethodServerTrust);
    EXPECT_NOT_NULL(challenge.protectionSpace.serverTrust);
    EXPECT_EQ(challenge.protectionSpace.port, port);
    EXPECT_WK_STREQ(challenge.protectionSpace.host, "127.0.0.1");
    verifyCertificateAndPublicKey(challenge.protectionSpace.serverTrust);
}

// FIXME: Fix WebTransportServer constructor and re-enable these tests once rdar://141009498 is available in OS builds.
TEST(WebTransport, DISABLED_ClientBidirectional)
{
    WebTransportServer echoServer([](ConnectionGroup group) -> Task {
        auto connection = co_await group.receiveIncomingConnection();
        auto request = co_await connection.awaitableReceiveBytes();
        co_await connection.awaitableSend(WTFMove(request));
    });

    auto configuration = adoptNS([WKWebViewConfiguration new]);
    enableWebTransport(configuration.get());
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration.get()]);
    auto delegate = adoptNS([TestNavigationDelegate new]);
    [webView setNavigationDelegate:delegate.get()];
    __block bool challenged { false };
    __block uint16_t port = echoServer.port();
    delegate.get().didReceiveAuthenticationChallenge = ^(WKWebView *, NSURLAuthenticationChallenge *challenge, void (^completionHandler)(NSURLSessionAuthChallengeDisposition, NSURLCredential *)) {
        validateChallenge(challenge, port);
        challenged = true;
        completionHandler(NSURLSessionAuthChallengeUseCredential, [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust]);
    };

    NSString *html = [NSString stringWithFormat:@""
        "<script>async function test() {"
        "  try {"
        "    let t = new WebTransport('https://127.0.0.1:%d/');"
        "    await t.ready;"
        "    let s = await t.createBidirectionalStream();"
        "    let w = s.writable.getWriter();"
        "    await w.write(new TextEncoder().encode('abc'));"
        "    let r = s.readable.getReader();"
        "    const { value, done } = await r.read();"
        "    alert('successfully read ' + new TextDecoder().decode(value));"
        "  } catch (e) { alert('caught ' + e); }"
        "}; test();"
        "</script>",
        port];
    [webView loadHTMLString:html baseURL:[NSURL URLWithString:@"https://webkit.org/"]];
    EXPECT_WK_STREQ([webView _test_waitForAlert], "successfully read abc");
    EXPECT_TRUE(challenged);
}

// FIXME: Fix WebTransportServer constructor and re-enable these tests once rdar://141009498 is available in OS builds.
TEST(WebTransport, DISABLED_Datagram)
{
    WebTransportServer echoServer([](ConnectionGroup group) -> Task {
        auto datagramConnection = group.createWebTransportConnection(ConnectionGroup::ConnectionType::Datagram);
        auto request = co_await datagramConnection.awaitableReceiveBytes();
        co_await datagramConnection.awaitableSend(WTFMove(request));
    });

    auto configuration = adoptNS([WKWebViewConfiguration new]);
    enableWebTransport(configuration.get());
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration.get()]);
    auto delegate = adoptNS([TestNavigationDelegate new]);
    [webView setNavigationDelegate:delegate.get()];
    __block bool challenged { false };
    __block uint16_t port = echoServer.port();
    delegate.get().didReceiveAuthenticationChallenge = ^(WKWebView *, NSURLAuthenticationChallenge *challenge, void (^completionHandler)(NSURLSessionAuthChallengeDisposition, NSURLCredential *)) {
        validateChallenge(challenge, port);
        challenged = true;
        completionHandler(NSURLSessionAuthChallengeUseCredential, [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust]);
    };

    NSString *html = [NSString stringWithFormat:@""
        "<script>async function test() {"
        "  try {"
        "    let t = new WebTransport('https://127.0.0.1:%d/');"
        "    await t.ready;"
        "    let w = t.datagrams.writable.getWriter();"
        "    await w.write(new TextEncoder().encode('abc'));"
        "    let r = t.datagrams.readable.getReader();"
        "    const { value, done } = await r.read();"
        "    alert('successfully read ' + new TextDecoder().decode(value));"
        "  } catch (e) { alert('caught ' + e); }"
        "}; test();"
        "</script>",
        port];
    [webView loadHTMLString:html baseURL:[NSURL URLWithString:@"https://webkit.org/"]];
    EXPECT_WK_STREQ([webView _test_waitForAlert], "successfully read abc");
    EXPECT_TRUE(challenged);
}

// FIXME: Fix WebTransportServer constructor and re-enable these tests once rdar://141009498 is available in OS builds.
TEST(WebTransport, DISABLED_Unidirectional)
{
    WebTransportServer echoServer([](ConnectionGroup group) -> Task {
        auto connection = co_await group.receiveIncomingConnection();
        auto request = co_await connection.awaitableReceiveBytes();
        auto serverUnidirectionalStream = group.createWebTransportConnection(ConnectionGroup::ConnectionType::Unidirectional);
        co_await serverUnidirectionalStream.awaitableSend(WTFMove(request));
    });

    auto configuration = adoptNS([WKWebViewConfiguration new]);
    enableWebTransport(configuration.get());
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration.get()]);
    auto delegate = adoptNS([TestNavigationDelegate new]);
    [webView setNavigationDelegate:delegate.get()];
    __block bool challenged { false };
    __block uint16_t port = echoServer.port();
    delegate.get().didReceiveAuthenticationChallenge = ^(WKWebView *, NSURLAuthenticationChallenge *challenge, void (^completionHandler)(NSURLSessionAuthChallengeDisposition, NSURLCredential *)) {
        validateChallenge(challenge, port);
        challenged = true;
        completionHandler(NSURLSessionAuthChallengeUseCredential, [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust]);
    };

    NSString *html = [NSString stringWithFormat:@""
        "<script>async function test() {"
        "  try {"
        "    let t = new WebTransport('https://127.0.0.1:%d/');"
        "    await t.ready;"
        "    let c = await t.createUnidirectionalStream();"
        "    let w = c.getWriter();"
        "    await w.write(new TextEncoder().encode('abc'));"
        "    let sr = t.incomingUnidirectionalStreams.getReader();"
        "    let {value: s, d} = await sr.read();"
        "    let r = s.getReader();"
        "    const { value, done } = await r.read();"
        "    alert('successfully read ' + new TextDecoder().decode(value));"
        "  } catch (e) { alert('caught ' + e); }"
        "}; test();"
        "</script>",
        port];
    [webView loadHTMLString:html baseURL:[NSURL URLWithString:@"https://webkit.org/"]];
    EXPECT_WK_STREQ([webView _test_waitForAlert], "successfully read abc");
    EXPECT_TRUE(challenged);
}

// FIXME: Fix WebTransportServer constructor and re-enable these tests once rdar://141009498 is available in OS builds.
TEST(WebTransport, DISABLED_ServerBidirectional)
{
    WebTransportServer echoServer([](ConnectionGroup group) -> Task {
        auto connection = co_await group.receiveIncomingConnection();
        auto request = co_await connection.awaitableReceiveBytes();
        auto serverBidirectionalStream = group.createWebTransportConnection(ConnectionGroup::ConnectionType::Bidirectional);
        co_await serverBidirectionalStream.awaitableSend(WTFMove(request));
    });

    auto configuration = adoptNS([WKWebViewConfiguration new]);
    enableWebTransport(configuration.get());
    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectZero configuration:configuration.get()]);
    auto delegate = adoptNS([TestNavigationDelegate new]);
    [webView setNavigationDelegate:delegate.get()];
    __block bool challenged { false };
    __block uint16_t port = echoServer.port();
    delegate.get().didReceiveAuthenticationChallenge = ^(WKWebView *, NSURLAuthenticationChallenge *challenge, void (^completionHandler)(NSURLSessionAuthChallengeDisposition, NSURLCredential *)) {
        validateChallenge(challenge, port);
        challenged = true;
        completionHandler(NSURLSessionAuthChallengeUseCredential, [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust]);
    };

    NSString *html = [NSString stringWithFormat:@""
        "<script>async function test() {"
        "  try {"
        "    let t = new WebTransport('https://127.0.0.1:%d/');"
        "    await t.ready;"
        "    let c = await t.createBidirectionalStream();"
        "    let w = c.writable.getWriter();"
        "    await w.write(new TextEncoder().encode('abc'));"
        "    let sr = t.incomingBidirectionalStreams.getReader();"
        "    let {value: s, d} = await sr.read();"
        "    let r = s.readable.getReader();"
        "    const { value, done } = await r.read();"
        "    alert('successfully read ' + new TextDecoder().decode(value));"
        "  } catch (e) { alert('caught ' + e); }"
        "}; test();"
        "</script>",
        port];
    [webView loadHTMLString:html baseURL:[NSURL URLWithString:@"https://webkit.org/"]];
    EXPECT_WK_STREQ([webView _test_waitForAlert], "successfully read abc");
    EXPECT_TRUE(challenged);
}
} // namespace TestWebKitAPI

#endif // HAVE(WEB_TRANSPORT)
