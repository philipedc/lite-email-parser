{
  "variables": {
    "src_root%": "<!(node -e \"var fs=require('fs');console.log(fs.existsSync('core')?'./':'../')\")"
  },
  "targets": [
    {
      "target_name": "parser",
      "sources": [
        "src/addon.cpp",
        "<(src_root)core/src/html_parser.cpp",
        "<(src_root)core/src/html_extractor.cpp",
        "<(src_root)deps/gumbo-parser/src/attribute.c",
        "<(src_root)deps/gumbo-parser/src/char_ref.c",
        "<(src_root)deps/gumbo-parser/src/char_ref_gperf.c",
        "<(src_root)deps/gumbo-parser/src/error.c",
        "<(src_root)deps/gumbo-parser/src/parser.c",
        "<(src_root)deps/gumbo-parser/src/string_buffer.c",
        "<(src_root)deps/gumbo-parser/src/string_piece.c",
        "<(src_root)deps/gumbo-parser/src/tag.c",
        "<(src_root)deps/gumbo-parser/src/tokenizer.c",
        "<(src_root)deps/gumbo-parser/src/utf8.c",
        "<(src_root)deps/gumbo-parser/src/util.c",
        "<(src_root)deps/gumbo-parser/src/vector.c"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(src_root)core/include",
        "<(src_root)deps/gumbo-parser/src",
        "<(src_root)deps/gumbo-parser/compat"
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
