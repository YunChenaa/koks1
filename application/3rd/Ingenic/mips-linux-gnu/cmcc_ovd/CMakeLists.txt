
add_definitions(-DOVD_BUILDIN)

add_library( cmcc_ovd STATIC
	./src/wrapper.c
	./src/inirw.c
	./src/demoAVPushing.c
	./src/demoAVPushing_util.c
	./src/OVD_DemoCallBack.c
	./src/OVD_DEMO_FirmwareUpgrade.c
	./src/OVD_DemoMain.c
	./src/OVD_DEMOVOICE.c
	./src/OVD_DEMO_ChangeCodec.c
	./src/OVD_DemoHibernation.c
	./src/OVD_Demo_SetConfig.c
	./src/OVD_DemoCmd.c
	./src/OVD_DemoInit.c
	./src/OVD_DemoTime.c )

target_include_directories(cmcc_ovd PRIVATE ./include)

target_link_libraries(cmcc_ovd PRIVATE atomic ${LIB_OVDSDK})
