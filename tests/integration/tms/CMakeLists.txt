add_test(NAME tms_web_help COMMAND $<TARGET_FILE:tms_web> --help)
set_tests_properties(tms_web_help PROPERTIES LABELS "integration;smoke;tms")
