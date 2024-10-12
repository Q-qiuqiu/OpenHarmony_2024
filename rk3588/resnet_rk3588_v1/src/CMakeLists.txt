find_package(OpenCV REQUIRED)
add_executable(resnet_infer
        main.cpp
        model.cpp
        resnet.cpp
)

target_include_directories(resnet_infer PRIVATE ${CMAKE_SOURCE_DIR}/lib/rknn/include)
target_link_directories(resnet_infer PRIVATE ${CMAKE_SOURCE_DIR}/lib/rknn/lib)

target_link_libraries(resnet_infer PRIVATE rknnrt ${OpenCV_LIBS})

install(TARGETS resnet_infer DESTINATION bin)
install(FILES ${CMAKE_SOURCE_DIR}/model/resnet.rknn DESTINATION model)
install(FILES ${CMAKE_SOURCE_DIR}/lib/rknn/lib/librknnrt.so DESTINATION lib)
