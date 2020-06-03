#include "V8Wrapper.h"
#include "Helper.h"
#include <assert.h>

void Print(const v8::FunctionCallbackInfo<v8::Value>& args);
void Read(const v8::FunctionCallbackInfo<v8::Value>& args);
void Load(const v8::FunctionCallbackInfo<v8::Value>& args);
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args);
void Version(const v8::FunctionCallbackInfo<v8::Value>& args);

void V8Wrapper::runScript(std::string fileName)
{
    v8::Isolate* isolate = context->GetIsolate();
	
    v8::Local<v8::String> file_name =
        v8::String::NewFromUtf8(isolate, "test.js", v8::NewStringType::kNormal)
        .ToLocalChecked();

    v8::Local<v8::String> source;

    if (!Helper::ReadFile(isolate, "test.js").ToLocal(&source)) {
        fprintf(stderr, "Error reading '%s'\n", "test.js");
        //continue;
    }

    bool success = ExecuteString(isolate, source, file_name, true, true);
    while (v8::platform::PumpMessageLoop(platform.get(), isolate)) {
        continue;
    }

    if (success) {
        throw "AHHHHHHHHHHHHHHHHH failed to run";
    }
}

void V8Wrapper::startV8(char* dir)
{
    v8::V8::InitializeICUDefaultLocation(dir);
    v8::V8::InitializeExternalStartupData(dir);
    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
	
    //v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
	
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);

    context = CreateShellContext(isolate);
	
    if (context.IsEmpty()) {
        fprintf(stderr, "Error creating context\n");
        throw "SHIT FUCKED";
    }
	
    v8::Context::Scope context_scope(context);
}

void V8Wrapper::shutdownV8()
{
    // isolate->Dispose();

    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
}

// Creates a new execution environment containing the built-in
// functions.
v8::Local<v8::Context> V8Wrapper::CreateShellContext(v8::Isolate* isolate)
{
    // Create a template for the global object.
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
	
    // Bind the global 'print' function to the C++ Print callback.
    global->Set(
        v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal).ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, Print)
    );
	
    // Bind the global 'read' function to the C++ Read callback.
    global->Set(v8::String::NewFromUtf8(
        isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, Read)
    );
	
    // Bind the global 'load' function to the C++ Load callback.
    global->Set(v8::String::NewFromUtf8(
        isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, Load)
    );
	
    // Bind the 'quit' function
    global->Set(v8::String::NewFromUtf8(
        isolate, "quit", v8::NewStringType::kNormal).ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, Quit)
    );
	
    // Bind the 'version' function
    global->Set(
        v8::String::NewFromUtf8(isolate, "version", v8::NewStringType::kNormal).ToLocalChecked(),
        v8::FunctionTemplate::New(isolate, Version)
    );
	
    return v8::Context::New(isolate, NULL, global);
}
// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value>& args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        if (first) {
            first = false;
        }
        else {
            printf(" ");
        }
        v8::String::Utf8Value str(args.GetIsolate(), args[i]);
        const char* cstr = Helper::ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}
// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value>& args) {
    if (args.Length() != 1) {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
                v8::NewStringType::kNormal).ToLocalChecked());
        return;
    }
    v8::String::Utf8Value file(args.GetIsolate(), args[0]);
    if (*file == NULL) {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                v8::NewStringType::kNormal).ToLocalChecked());
        return;
    }
    v8::Local<v8::String> source;
    if (!Helper::ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
        args.GetIsolate()->ThrowException(
            v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                v8::NewStringType::kNormal).ToLocalChecked());
        return;
    }
    args.GetReturnValue().Set(source);
}
// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void Load(const v8::FunctionCallbackInfo<v8::Value>& args) {
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        v8::String::Utf8Value file(args.GetIsolate(), args[i]);
        if (*file == NULL) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                    v8::NewStringType::kNormal).ToLocalChecked());
            return;
        }
        v8::Local<v8::String> source;
        if (!Helper::ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                    v8::NewStringType::kNormal).ToLocalChecked());
            return;
        }
        /*if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
            args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
                    v8::NewStringType::kNormal).ToLocalChecked());
            return;
        }*/
    }
}
// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void Quit(const v8::FunctionCallbackInfo<v8::Value>& args) {
    // If not arguments are given args[0] will yield undefined which
    // converts to the integer value 0.
    int exit_code =
        args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    fflush(stdout);
    fflush(stderr);
    exit(exit_code);
}
void Version(const v8::FunctionCallbackInfo<v8::Value>& args) {
    args.GetReturnValue().Set(
        v8::String::NewFromUtf8(args.GetIsolate(), v8::V8::GetVersion(),
            v8::NewStringType::kNormal).ToLocalChecked());
}
// Reads a file into a v8 string.
//v8::MaybeLocal<v8::String> V8Wrapper::ReadFile(v8::Isolate* isolate, const char* name) {
//    v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
//        isolate, Helper::getFileContents("test.js").c_str(), v8::NewStringType::kNormal);
//    return result;
//}

// Executes a string within the current v8 context.
bool V8Wrapper::ExecuteString(v8::Isolate* isolate, v8::Local<v8::String> source,
    v8::Local<v8::Value> name, bool print_result,
    bool report_exceptions) {
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::ScriptOrigin origin(name);
    v8::Local<v8::Context> context(isolate->GetCurrentContext());
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
        // Print errors that happened during compilation.
        if (report_exceptions)
            ReportException(isolate, &try_catch);
        return false;
    }
    else {
        v8::Local<v8::Value> result;
        if (!script->Run(context).ToLocal(&result)) {
            assert(try_catch.HasCaught());
            // Print errors that happened during execution.
            if (report_exceptions)
                ReportException(isolate, &try_catch);
            return false;
        }
        else {
            assert(!try_catch.HasCaught());
            if (print_result && !result->IsUndefined()) {
                // If all went well and the result wasn't undefined then print
                // the returned value.
                v8::String::Utf8Value str(isolate, result);
                const char* cstr = Helper::ToCString(str);
                printf("%s\n", cstr);
            }
            return true;
        }
    }
}

void V8Wrapper::ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch) {
    v8::HandleScope handle_scope(isolate);
    v8::String::Utf8Value exception(isolate, try_catch->Exception());

    const char* exception_string = Helper::ToCString(exception);
    v8::Local<v8::Message> message = try_catch->Message();

    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        fprintf(stderr, "%s\n", exception_string);
    }
    else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(isolate,
            message->GetScriptOrigin().ResourceName());
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        const char* filename_string = Helper::ToCString(filename);
        int linenum = message->GetLineNumber(context).FromJust();
        fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);

        // Print line of source code.
        v8::String::Utf8Value sourceline(
            isolate, message->GetSourceLine(context).ToLocalChecked());
        const char* sourceline_string = Helper::ToCString(sourceline);
        fprintf(stderr, "%s\n", sourceline_string);

        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }

        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }

        fprintf(stderr, "\n");
        v8::Local<v8::Value> stack_trace_string;

        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
            stack_trace_string->IsString() &&
            v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
            v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
            const char* stack_trace_string = Helper::ToCString(stack_trace);
            fprintf(stderr, "%s\n", stack_trace_string);
        }
    }
}