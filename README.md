# aruco

Чтобы запустить различные части задания можно менять значение переменной `int labCurrentPart`. 
- Сгенерировать маркер (`labCurrentPart = GENERATE_MARKER`), заранее выбрав словать `int dictionaryName`, а так же размеры и количество маркеров `int markersX, markersY, markerLength, markerSeparation`.
- Калибровать камеру (`labCurrentPart = CALIBRATE_CAMERA`) по паттерну шахмантая доска 9 на 6 клеток. Размер одной клетки желательно 27 мм. Результаты калибровки записаны в camera.xml.
- Детектировать маркеры (`labCurrentPart = DETECT_MARKER`) заранее передав программе в качестве параметра путь до фотографии с маркерами. 
