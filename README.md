# veeam-signature

## V1
### Входные данные

- --input=< /path/to/input/file > - Путь до хешируемого файла
- --output=< /path/to/output/file > - Путь до файла подписи
- --chunk-size=< file_size > - размер блока, опционален. допустимо указывать суффиксом единицу измерения, например 4Mib. 
Отсутствие суффикса интерпретируется как байты, допустимые суффиксы: kb, kib, mb, mib, gb, gib.
Допустимые размеры чанков от 64Kb до 1Gb.

### Решаемые проблемы

#### Проблема выходного файла (внезапно первая)

Поскольку минимальный размер блока 64Kb, то количество чанков для файла размером в 1Tb будет 15_625_000.
При размере записи хеша одного чанка к примеру в 100 байт(с запасом), общий размер выходного файла составит 15_625_000 * 100B / 1_000_000B = 1562,4Mb.
Таким образом, проблемы нет, и собрать все хеши чанков можно в памяти =). 

#### Проблема чтения файла не помещающегося в RAM

Есть несколько вариантов решения:

- Вызывать mmap, читать последовательно чанками, раскидывать их задачами на тредпул, потом сбрасывать на выход по готовности чанка, сортировать, писать в файл.
Плюсы: ничего не копируется, чтение строго последовательное (привет hdd). 
Минусы: постоянные page fault, но которые вроде как не критичные(надо разбираться)

- Вызывать mmap, порезать на блоки строго по количеству ядер, потом сбрасывать на выход по готовности блока, сортировать, писать в файл.
Плюсы: ничего не копируется, чтение строго последовательные по блокам(непонятно, хорошо или так себе)
Минусы: больше page fault, доступ уже все таки не линейный а произвольный

- Вызывать read, читать большими буферами (100mb), раскидывать на тредпул и т.д. как в первом варианте
Плюсы: строго линейное чтение, теоретически быстрее mmap
Минусы: копирование в юзерленд, но вроде довольно оптимальное

- Вызывать read, читать большими буферами (100mb), поделить файл на блоки, читать считать каждый своим потоком.
 Плюсы: чтение линейное в рамках потока
 Минусы: копирование в юзерленд, но вроде довольно оптимальное

В итоге сделан второй вариант, но в целом нужны более явные границы задачи, после чего нормальные измерения в ее границах


### Резюме

Задача решена в лоб, и именно в рамках ТЗ, в рабочем серверном приложении разумнее будет собрать конвейер чтения-обработки-записи,
например, с использованием boost::asio. Смарт-пойнтеры не потребовались(пункт ТЗ), но в случае asio без них никуда =)

Тестовое окружение: i7-6700(8core), 32Gb ram, ssd, debian-bullseye(testing), gcc 10.2

Результаты: хеширование 30Gb файла с размером чанка 64Kb со сброшенными кешами(reset_disk_cache.sh) и записью на диск - 50 секунд. 
При этом загрузка дисков(iotop) ~90%, скорость чтения с диска ~70MB/s, загрузка CPU(htop) ~15%(видимо уперся в то, как читаю с диска)

## V2

### Реализация

Файл разбивается на блоки по количеству ядер с выравниванием по размеру чанка, после чего выполняется обработка на пуле потоков.
Чтение блока производится сегментами ~128Mb c выравниванием на размер чанка, c последующей постановкой задачи хеширования сегмента в пул.
Захешированный сегмент помещается в хранилище, по окончании хеширования всех сегментов/блоков хеши записываются в выходной файл.

### Резюме

Смена принципа работы существенно повысила скорость работы, и уменьшила потребление памяти. 
Скорее всего деградация производительности v1 связана с большим количеством page fault и в принципе большом количестве необходимой памяти.
Полагаю, v2 можно было бы еще улучшить, если выполнять последовательно цикл чтение/хеширование в одном участке кода, и аллоцировать буфер для чтения единожды,
и аллоцировать память только для хешей.

Результаты:
- v1: Elapsed - 56.70 seconds, throughput is 554.79 Mb/sec
- v2: Elapsed - 10.13 seconds, throughput is 3104.18 Mb/sec

Нагрузка CPU ~80%,  потребление памяти ~5%, диск ~40%.