add_test(NAME playout_service_help COMMAND $<TARGET_FILE:playout_service> --help)
set_tests_properties(playout_service_help PROPERTIES LABELS "integration;smoke;playout")
