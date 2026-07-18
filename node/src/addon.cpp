#include <napi.h>
#include "html_parser.h"
#include "html_extractor.h"

Napi::String CleanHtmlWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }
    std::string html = info[0].As<Napi::String>().Utf8Value();
    std::string result = liteEmailParser::cleanHtml(html);
    return Napi::String::New(env, result);
}

Napi::String RemoveSignatureWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }
    std::string html = info[0].As<Napi::String>().Utf8Value();
    std::string result = liteEmailParser::removeSignature(html);
    return Napi::String::New(env, result);
}

Napi::String RemoveRepliesWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }
    std::string html = info[0].As<Napi::String>().Utf8Value();
    std::string result = liteEmailParser::removeReplies(html);
    return Napi::String::New(env, result);
}

Napi::String RemoveDividersWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
        return Napi::String::New(env, "");
    }
    std::string html = info[0].As<Napi::String>().Utf8Value();
    std::string result = liteEmailParser::removeDividers(html);
    return Napi::String::New(env, result);
}

Napi::Value ParseEmailWrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    if (info.Length() < 1 || !info[0].IsBuffer()) {
        Napi::TypeError::New(env, "Buffer expected").ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::Buffer<char> buffer = info[0].As<Napi::Buffer<char>>();
    std::string rawEmail(buffer.Data(), buffer.Length());

    liteEmailParser::ParseEmailResult result = liteEmailParser::parseEmail(rawEmail);
    std::string cleaned = liteEmailParser::cleanHtml(result.text);

    Napi::Object obj = Napi::Object::New(env);
    obj.Set("subject", Napi::String::New(env, result.subject));
    obj.Set("from", Napi::String::New(env, result.from));
    obj.Set("to", Napi::String::New(env, result.to));
    obj.Set("lastMessage", Napi::String::New(env, cleaned));

    Napi::Array attachments = Napi::Array::New(env, result.attachments.size());
    for (size_t i = 0; i < result.attachments.size(); ++i) {
        const auto& att = result.attachments[i];
        Napi::Object attObj = Napi::Object::New(env);
        attObj.Set("name", Napi::String::New(env, att.name));
        attObj.Set("type", Napi::String::New(env, att.type));
        attObj.Set("size", Napi::Number::New(env, att.size));
        attObj.Set("buffer", Napi::Buffer<char>::Copy(env, att.buffer.data(), att.buffer.size()));
        attachments.Set(i, attObj);
    }
    obj.Set("attachments", attachments);

    Napi::Array inlineAttachments = Napi::Array::New(env, result.inlineAttachments.size());
    for (size_t i = 0; i < result.inlineAttachments.size(); ++i) {
        const auto& att = result.inlineAttachments[i];
        Napi::Object attObj = Napi::Object::New(env);
        attObj.Set("name", Napi::String::New(env, att.name));
        attObj.Set("type", Napi::String::New(env, att.type));
        attObj.Set("size", Napi::Number::New(env, att.size));
        attObj.Set("buffer", Napi::Buffer<char>::Copy(env, att.buffer.data(), att.buffer.size()));
        if (!att.originalSrc.empty()) {
            attObj.Set("originalSrc", Napi::String::New(env, att.originalSrc));
        }
        if (!att.src.empty()) {
            attObj.Set("src", Napi::String::New(env, att.src));
        }
        inlineAttachments.Set(i, attObj);
    }
    obj.Set("inlineAttachments", inlineAttachments);

    return obj;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "cleanHtml"), Napi::Function::New(env, CleanHtmlWrapped));
    exports.Set(Napi::String::New(env, "removeSignature"), Napi::Function::New(env, RemoveSignatureWrapped));
    exports.Set(Napi::String::New(env, "removeReplies"), Napi::Function::New(env, RemoveRepliesWrapped));
    exports.Set(Napi::String::New(env, "removeDividers"), Napi::Function::New(env, RemoveDividersWrapped));
    exports.Set(Napi::String::New(env, "parseEmail"), Napi::Function::New(env, ParseEmailWrapped));
    return exports;
}

NODE_API_MODULE(parser, Init)
