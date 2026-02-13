if(CONFIG_ZMK_DISPLAY AND CONFIG_NICE_VIEW_WIDGET_STATUS)
  # 初始化库
  zephyr_library()

  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  
  # 添加公共源文件
  zephyr_library_sources(custom_status_screen.c)
  zephyr_library_sources(widgets/bolt.c)
  zephyr_library_sources(widgets/util.c)

  # 根据角色添加不同的源文件
  if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(widgets/status.c)
  else()
    zephyr_library_sources(widgets/art.c)
    zephyr_library_sources(widgets/peripheral_status.c)
  endif()

  # --- 关键修改：将本库链接到 ZMK 的主 app 逻辑 ---
  # 只有链接了 app，编译器才能找到 zmk_ble_... 和 zmk_wpm_... 等函数的实现
  zephyr_library_link_libraries(app)

endif()

