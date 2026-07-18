{
  "targets": [
    {
      "target_name": "parser",
      "sources": [
        "src/addon.cpp",
        "../core/src/html_parser.cpp",
        "../core/src/html_extractor.cpp",
        "../deps/gumbo-parser/src/attribute.c",
        "../deps/gumbo-parser/src/char_ref.c",
        "../deps/gumbo-parser/src/char_ref_gperf.c",
        "../deps/gumbo-parser/src/error.c",
        "../deps/gumbo-parser/src/parser.c",
        "../deps/gumbo-parser/src/string_buffer.c",
        "../deps/gumbo-parser/src/string_piece.c",
        "../deps/gumbo-parser/src/tag.c",
        "../deps/gumbo-parser/src/tokenizer.c",
        "../deps/gumbo-parser/src/utf8.c",
        "../deps/gumbo-parser/src/util.c",
        "../deps/gumbo-parser/src/vector.c"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "../core/include",
        "../deps/gumbo-parser/src"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').targets\"):node_addon_api"
      ],
      "libraries": [],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "xcode_settings": {
        "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.7"
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 }
      }
    }
  ]
}
