cmake_minimum_required(VERSION 3.28)
project(SackServer LANGUAGES C)

# 設定 C 標準 (C11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 強制 MSVC 使用 UTF-8 (解決中文亂碼)
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 輸出目錄設定 (預設放在 build/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- 1. 設定標頭檔路徑 ---
# 讓所有程式都能找到 include/libserv.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- 2. 編譯主伺服器 (sackServ) ---
# 包含所有必要的 .c 檔 (除了 apServ.c)
add_executable(sackServ
    src/main.c
    src/libserv.c
    src/sockwrap.c
    src/Queue.c
    src/roomOp.c
    src/gameplay.c
    src/errfunc.c
)

# --- 3. 編譯電腦玩家伺服器 (apServ) ---
add_executable(apServ
    src/apServ.c
)

# --- 4. Windows 平台設定 ---
if(WIN32)
    # 連結 Winsock 函式庫 (ws2_32)
    target_link_libraries(sackServ ws2_32)
    target_link_libraries(apServ ws2_32)
    
    # 定義 _WIN32 巨集 (雖然編譯器通常會自動加，但顯式定義保險)
    target_compile_definitions(sackServ PRIVATE _WIN32)
    target_compile_definitions(apServ PRIVATE _WIN32)
endif()

# --- 5. 自動複製執行檔到根目錄 (選用) ---
# 這樣編譯完後，exe 會直接出現在你的專案資料夾，不用進 build 找
add_custom_command(TARGET sackServ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:sackServ> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:sackServ>
    COMMENT "Copying sackServ to root directory"
)

add_custom_command(TARGET apServ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:apServ> ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:apServ>
    COMMENT "Copying apServ to root directory"
)