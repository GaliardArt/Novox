include(FetchContent)
set(NOVOX_RNNOISE_REVISION 70f1d256acd4b34a572f999a05c87bf00b67730d)
set(NOVOX_RNNOISE_MODEL_SHA256 0a8755f8e2d834eff6a54714ecc7d75f9932e845df35f8b59bc52a7cfe6e8b37)
FetchContent_Declare(rnnoise GIT_REPOSITORY https://github.com/xiph/rnnoise.git GIT_TAG ${NOVOX_RNNOISE_REVISION} GIT_SHALLOW FALSE)
FetchContent_GetProperties(rnnoise)
if(NOT rnnoise_POPULATED)
  FetchContent_Populate(rnnoise)
endif()
set(_model_archive ${CMAKE_BINARY_DIR}/_deps/rnnoise-model-${NOVOX_RNNOISE_MODEL_SHA256}.tar.gz)
set(_model_dir ${CMAKE_BINARY_DIR}/_deps/rnnoise-model-src)
if(NOT EXISTS ${_model_archive})
  file(DOWNLOAD https://media.xiph.org/rnnoise/models/rnnoise_data-${NOVOX_RNNOISE_MODEL_SHA256}.tar.gz ${_model_archive} EXPECTED_HASH SHA256=${NOVOX_RNNOISE_MODEL_SHA256} TLS_VERIFY ON STATUS _download_status)
  list(GET _download_status 0 _download_code)
  if(NOT _download_code EQUAL 0)
    list(GET _download_status 1 _download_error)
    message(FATAL_ERROR RNNoise_model_download_failed:_${_download_error})
  endif()
endif()
if(NOT EXISTS ${_model_dir}/rnnoise_data.h)
  file(REMOVE_RECURSE ${_model_dir})
  file(MAKE_DIRECTORY ${_model_dir})
  file(ARCHIVE_EXTRACT INPUT ${_model_archive} DESTINATION ${_model_dir})
endif()
file(GLOB_RECURSE _model_c ${_model_dir}/rnnoise_data.c)
file(GLOB_RECURSE _model_h ${_model_dir}/rnnoise_data.h)
if(NOT _model_c OR NOT _model_h)
  message(FATAL_ERROR The_pinned_RNNoise_model_archive_is_missing_generated_model_sources)
endif()
list(GET _model_c 0 _model_c_file)
get_filename_component(_model_include ${_model_h} DIRECTORY)
add_library(NOVOX_RNNoise STATIC
  ${rnnoise_SOURCE_DIR}/src/denoise.c
  ${rnnoise_SOURCE_DIR}/src/rnn.c
  ${rnnoise_SOURCE_DIR}/src/pitch.c
  ${rnnoise_SOURCE_DIR}/src/kiss_fft.c
  ${rnnoise_SOURCE_DIR}/src/celt_lpc.c
  ${rnnoise_SOURCE_DIR}/src/nnet.c
  ${rnnoise_SOURCE_DIR}/src/nnet_default.c
  ${rnnoise_SOURCE_DIR}/src/parse_lpcnet_weights.c
  ${rnnoise_SOURCE_DIR}/src/rnnoise_tables.c
  ${_model_c_file})
target_include_directories(NOVOX_RNNoise PUBLIC ${rnnoise_SOURCE_DIR}/include PRIVATE ${rnnoise_SOURCE_DIR}/src ${_model_include})
