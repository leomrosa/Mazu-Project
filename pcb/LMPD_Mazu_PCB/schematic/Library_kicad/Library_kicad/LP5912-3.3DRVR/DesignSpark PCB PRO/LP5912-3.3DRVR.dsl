SamacSys ECAD Model
355048/1207301/2.50/7/4/Integrated Circuit

DESIGNSPARK_INTERMEDIATE_ASCII

(asciiHeader
	(fileUnits MM)
)
(library Library_1
	(padStyleDef "r60_35"
		(holeDiam 0)
		(padShape (layerNumRef 1) (padShapeType Rect)  (shapeWidth 0.35) (shapeHeight 0.6))
		(padShape (layerNumRef 16) (padShapeType Ellipse)  (shapeWidth 0) (shapeHeight 0))
	)
	(padStyleDef "r170_110"
		(holeDiam 0)
		(padShape (layerNumRef 1) (padShapeType Rect)  (shapeWidth 1.1) (shapeHeight 1.7))
		(padShape (layerNumRef 16) (padShapeType Ellipse)  (shapeWidth 0) (shapeHeight 0))
	)
	(textStyleDef "Default"
		(font
			(fontType Stroke)
			(fontFace "Helvetica")
			(fontHeight 50 mils)
			(strokeWidth 5 mils)
		)
	)
	(patternDef "SON65P200X200X80-7N" (originalName "SON65P200X200X80-7N")
		(multiLayer
			(pad (padNum 1) (padStyleRef r60_35) (pt -1.05, 0.65) (rotation 90))
			(pad (padNum 2) (padStyleRef r60_35) (pt -1.05, 0) (rotation 90))
			(pad (padNum 3) (padStyleRef r60_35) (pt -1.05, -0.65) (rotation 90))
			(pad (padNum 4) (padStyleRef r60_35) (pt 1.05, -0.65) (rotation 90))
			(pad (padNum 5) (padStyleRef r60_35) (pt 1.05, 0) (rotation 90))
			(pad (padNum 6) (padStyleRef r60_35) (pt 1.05, 0.65) (rotation 90))
			(pad (padNum 7) (padStyleRef r170_110) (pt 0, 0) (rotation 0))
		)
		(layerContents (layerNumRef 18)
			(attr "RefDes" "RefDes" (pt 0, 0) (textStyleRef "Default") (isVisible True))
		)
		(layerContents (layerNumRef 30)
			(line (pt -1.6 1.3) (pt 1.6 1.3) (width 0.05))
		)
		(layerContents (layerNumRef 30)
			(line (pt 1.6 1.3) (pt 1.6 -1.3) (width 0.05))
		)
		(layerContents (layerNumRef 30)
			(line (pt 1.6 -1.3) (pt -1.6 -1.3) (width 0.05))
		)
		(layerContents (layerNumRef 30)
			(line (pt -1.6 -1.3) (pt -1.6 1.3) (width 0.05))
		)
		(layerContents (layerNumRef 28)
			(line (pt -1 1) (pt 1 1) (width 0.1))
		)
		(layerContents (layerNumRef 28)
			(line (pt 1 1) (pt 1 -1) (width 0.1))
		)
		(layerContents (layerNumRef 28)
			(line (pt 1 -1) (pt -1 -1) (width 0.1))
		)
		(layerContents (layerNumRef 28)
			(line (pt -1 -1) (pt -1 1) (width 0.1))
		)
		(layerContents (layerNumRef 28)
			(line (pt -1 0.5) (pt -0.5 1) (width 0.1))
		)
		(layerContents (layerNumRef 18)
			(arc (pt -1.35, 1.325) (radius 0.125) (startAngle 0.0) (sweepAngle 0.0) (width 0.25))
		)
		(layerContents (layerNumRef 18)
			(arc (pt -1.35, 1.325) (radius 0.125) (startAngle 180.0) (sweepAngle 180.0) (width 0.25))
		)
	)
	(symbolDef "LP5912-3_3DRVR" (originalName "LP5912-3_3DRVR")

		(pin (pinNum 1) (pt 0 mils 0 mils) (rotation 0) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 230 mils -25 mils) (rotation 0]) (justify "Left") (textStyleRef "Default"))
		))
		(pin (pinNum 2) (pt 0 mils -100 mils) (rotation 0) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 230 mils -125 mils) (rotation 0]) (justify "Left") (textStyleRef "Default"))
		))
		(pin (pinNum 3) (pt 0 mils -200 mils) (rotation 0) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 230 mils -225 mils) (rotation 0]) (justify "Left") (textStyleRef "Default"))
		))
		(pin (pinNum 4) (pt 500 mils -700 mils) (rotation 90) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 525 mils -470 mils) (rotation 90]) (justify "Left") (textStyleRef "Default"))
		))
		(pin (pinNum 5) (pt 1000 mils 0 mils) (rotation 180) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 770 mils -25 mils) (rotation 0]) (justify "Right") (textStyleRef "Default"))
		))
		(pin (pinNum 6) (pt 1000 mils -100 mils) (rotation 180) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 770 mils -125 mils) (rotation 0]) (justify "Right") (textStyleRef "Default"))
		))
		(pin (pinNum 7) (pt 1000 mils -200 mils) (rotation 180) (pinLength 200 mils) (pinDisplay (dispPinName false)) (pinName (text (pt 770 mils -225 mils) (rotation 0]) (justify "Right") (textStyleRef "Default"))
		))
		(line (pt 200 mils 100 mils) (pt 800 mils 100 mils) (width 6 mils))
		(line (pt 800 mils 100 mils) (pt 800 mils -500 mils) (width 6 mils))
		(line (pt 800 mils -500 mils) (pt 200 mils -500 mils) (width 6 mils))
		(line (pt 200 mils -500 mils) (pt 200 mils 100 mils) (width 6 mils))
		(attr "RefDes" "RefDes" (pt 850 mils 300 mils) (justify Left) (isVisible True) (textStyleRef "Default"))

	)
	(compDef "LP5912-3.3DRVR" (originalName "LP5912-3.3DRVR") (compHeader (numPins 7) (numParts 1) (refDesPrefix IC)
		)
		(compPin "1" (pinName "OUT") (partNum 1) (symPinNum 1) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(compPin "2" (pinName "NC") (partNum 1) (symPinNum 2) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(compPin "3" (pinName "PG") (partNum 1) (symPinNum 3) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(compPin "7" (pinName "EP") (partNum 1) (symPinNum 4) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(compPin "6" (pinName "IN") (partNum 1) (symPinNum 5) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(compPin "5" (pinName "GND") (partNum 1) (symPinNum 6) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(compPin "4" (pinName "EN") (partNum 1) (symPinNum 7) (gateEq 0) (pinEq 0) (pinType Bidirectional))
		(attachedSymbol (partNum 1) (altType Normal) (symbolName "LP5912-3_3DRVR"))
		(attachedPattern (patternNum 1) (patternName "SON65P200X200X80-7N")
			(numPads 7)
			(padPinMap
				(padNum 1) (compPinRef "1")
				(padNum 2) (compPinRef "2")
				(padNum 3) (compPinRef "3")
				(padNum 4) (compPinRef "4")
				(padNum 5) (compPinRef "5")
				(padNum 6) (compPinRef "6")
				(padNum 7) (compPinRef "7")
			)
		)
		(attr "Mouser Part Number" "595-LP5912-3.3DRVR")
		(attr "Mouser Price/Stock" "https://www.mouser.co.uk/ProductDetail/Texas-Instruments/LP5912-3.3DRVR?qs=n%252BTO0c4TDSY0RMrHlK%2FGvQ%3D%3D")
		(attr "Manufacturer_Name" "Texas Instruments")
		(attr "Manufacturer_Part_Number" "LP5912-3.3DRVR")
		(attr "Description" "500-mA low-noise low-IQ low-dropout (LDO) linear regulator")
		(attr "Datasheet Link" "http://www.ti.com/lit/gpn/lp5912")
		(attr "Height" "0.8 mm")
	)

)
