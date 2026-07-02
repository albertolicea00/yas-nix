qt_add_executable(tst_processrunner tst_processrunner.cpp)
target_link_libraries(tst_processrunner PRIVATE yascore Qt6::Test)
add_test(NAME tst_processrunner COMMAND tst_processrunner)

qt_add_executable(tst_packagelistmodel tst_packagelistmodel.cpp)
target_link_libraries(tst_packagelistmodel PRIVATE yascore Qt6::Test)
add_test(NAME tst_packagelistmodel COMMAND tst_packagelistmodel)

qt_add_executable(tst_nixadapter tst_nixadapter.cpp ../src/nixadapter.cpp)
target_include_directories(tst_nixadapter PRIVATE ../src)
target_link_libraries(tst_nixadapter PRIVATE yascore Qt6::Test)
add_test(NAME tst_nixadapter COMMAND tst_nixadapter)
