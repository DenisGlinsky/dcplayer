add_test(NAME sms_ui_help COMMAND $<TARGET_FILE:sms_ui> --help)
set_tests_properties(sms_ui_help PROPERTIES LABELS "integration;smoke;sms")
