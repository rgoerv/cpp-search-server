# Поисковый сервер

Выполняет поиск по полученному запросу с каждым документом в базе, определяет релевантность документа по частоте слов запроса в документе. Полученные результаты сортируются по релевантности, делятся на страницы, дупликаты удаляются. Имеется поддержка многопоточных вычислений, поэтому поиск можно выполнять в многопоточном режиме.

# Как начать

1. Проверить, все ли системные требования соблюдены.
2. Необходимо клонировать репозиторий и перейти в папку с ним.
3. Запустить сборку Makefile командой make. При запуске исполняемого файла без аргументов выполняется пример, а при запуске c любыми аргументами выполняются тесты.

# Системные требования

1. C++17
2. GCC 11.4.0
3. *Наличие вспомогалтельной библиотеки Thread Building Blocks(libtbb-dev) для UNIX систем.

# План по доработке

1. Применить для реализации тестов Boost.Test.
