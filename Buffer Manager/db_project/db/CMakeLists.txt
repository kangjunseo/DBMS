# Sources
set(DB_SOURCE_DIR src)
set(DB_SOURCES
  ${DB_SOURCE_DIR}/file/file.cc
  ${DB_SOURCE_DIR}/index/bpt.cc
  ${DB_SOURCE_DIR}/index/index.cc
  ${DB_SOURCE_DIR}/buffer/buffer.cc
  ${DB_SOURCE_DIR}/api/api.cc
  # Add your sources here
  # ${DB_SOURCE_DIR}/foo/bar/your_source.cc
  )

# Headers
set(DB_HEADER_DIR include)
set(DB_HEADERS
  ${DB_HEADER_DIR}/page.h
  ${DB_HEADER_DIR}/file.h
  ${DB_HEADER_DIR}/bpt.h
  ${DB_HEADER_DIR}/api.h
  ${DB_HEADER_DIR}/msg.h
  ${DB_HEADER_DIR}/buffer.h
  # Add your headers here
  # ${DB_HEADER_DIR}/foo/bar/your_header.h
  )

add_library(db STATIC ${DB_HEADERS} ${DB_SOURCES})

target_include_directories(db
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${DB_HEADER_DIR}"
  )

