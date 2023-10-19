all:
	g++-12 ./search-server/main.cpp ./search-server/document.cpp ./search-server/process_queries.cpp ./search-server/read_input_functions.cpp ./search-server/remove_duplicates.cpp ./search-server/request_queue.cpp ./search-server/search_server.cpp ./search-server/string_processing.cpp ./search-server/generator.cpp ./search-server/tests.cpp -o search_server --std=c++17 -ltbb -lpthread -O2	
