# 这里的 if 条件保持你原来的不变
if(CONFIG_ZMK_DISPLAY AND CONFIG_NICE_VIEW_WIDGET_STATUS)
  # 这一行非常重要，它初始化当前目录的库
  zephyr_library()

  zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)
  zephyr_library_sources(custom_status_screen.c)
  zephyr_library_sources(widgets/bolt.c)
  zephyr_library_sources(widgets/util.c)

  if(NOT CONFIG_ZMK_SPLIT OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    zephyr_library_sources(widgets/status.c)
  else()
    zephyr_library_sources(widgets/art.c)
    zephyr_library_sources(widgets/peripheral_status.c)
  endif()

  # 强制将这个 Shield 库与 ZMK 的核心 app 链接
  # 这样 ld (链接器) 才能找到 zmk_ble_... 等函数
  zephyr_library_link_libraries(app)
endif()
