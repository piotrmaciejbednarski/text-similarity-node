{
  "variables": {
    "openssl_fips": "0"
  },

  "target_defaults": {
    "include_dirs": [
      "<!@(node -p \"require('node-addon-api').include\")",
      "src"
    ],
    "dependencies": [
      "<!(node -p \"require('node-addon-api').gyp\")"
    ],
    "defines": [ "NDEBUG" ],
    "cflags_cc!": [ "-fno-exceptions" ],
    "cflags!":   [ "-fno-exceptions", "-fvisibility=hidden" ]
  },

  "targets": [
    {
      "target_name": "text_similarity_node",
      "sources": [
        "src/bindings/node_bindings.cpp",
        "src/core/unicode.cpp",
        "src/core/memory_pool.cpp",
        "src/core/algorithm_factory.cpp",
        "src/algorithms/base_algorithm.cpp",
        "src/algorithms/levenshtein.cpp",
        "src/algorithms/token_based.cpp",
        "src/algorithms/vector_based.cpp",
        "src/algorithms/phonetic.cpp",
        "src/engine/similarity_engine.cpp"
      ],

      "configurations": {
        "Release": {
          "cflags_cc+": [ "-O3", "-flto" ],
          "ldflags+":   [ "-flto" ],
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS": [ "-O3", "-flto" ],
            "OTHER_LDFLAGS":        [ "-flto" ]
          }
        },
        "Debug": {
          "cflags_cc+": [ "-O0", "-g" ],
          "defines+":   [ "DEBUG" ],
          "xcode_settings": {
            "OTHER_CPLUSPLUSFLAGS": [ "-O0", "-g" ]
          }
        }
      },

      "conditions": [
        [ "OS=='mac'", {
          "xcode_settings": {
            "MACOSX_DEPLOYMENT_TARGET": "10.14",
            "CLANG_CXX_LIBRARY": "libc++",
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "GCC_ENABLE_CPP_RTTI": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++17"
          },
          "conditions": [
            [ "target_arch=='x64'", {
              "xcode_settings": {
                "OTHER_CPLUSPLUSFLAGS": [ "-mavx2", "-msse4.2" ]
              }
            }],
            [ "target_arch=='arm64'", {
              "xcode_settings": {
                "OTHER_CPLUSPLUSFLAGS": [ "-march=armv8-a+simd" ]
              }
            }]
          ]
        }],
        [ "OS=='win'", {
          "defines": [ "WIN32_LEAN_AND_MEAN", "_CRT_SECURE_NO_WARNINGS", "_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING" ],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "RuntimeTypeInfo": "true",
              "Optimization": 3,
              "WholeProgramOptimization": "true",
              "LanguageStandard": "stdcpp17"
            },
            "VCLinkerTool": {
              "LinkTimeCodeGeneration": 1
            }
          }
        }],
        [ "OS=='linux'", {
          "cflags_cc+": [ "-std=c++17", "-frtti", "-O3" ],
          "conditions": [
            [ "target_arch=='x64'", {
              "cflags_cc+": [ "-mavx2", "-msse4.2", "-march=native" ]
            }],
            [ "target_arch=='arm64'", {
              "cflags_cc+": [ "-march=armv8-a+simd" ]
            }],
            [ "target_arch=='arm'", {
              "cflags_cc+": [ "-march=armv7-a", "-mfpu=neon" ]
            }]
          ]
        }]
      ]
    }
  ]
}