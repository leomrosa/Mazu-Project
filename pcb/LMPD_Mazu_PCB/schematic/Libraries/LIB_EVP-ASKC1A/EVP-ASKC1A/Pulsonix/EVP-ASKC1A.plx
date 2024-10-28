PULSONIX_LIBRARY_ASCII "SamacSys ECAD Model"
//2012078/1207301/2.50/2/0/Switch

(asciiHeader
	(fileUnits MM)
)
(library Library_1
	(padStyleDef "r200_200"
		(holeDiam 0)
		(padShape (layerNumRef 1) (padShapeType Rect)  (shapeWidth 2.000) (shapeHeight 2.000))
		(padShape (layerNumRef 16) (padShapeType Ellipse)  (shapeWidth 0) (shapeHeight 0))
	)
	(textStyleDef "Normal"
		(font
			(fontType Stroke)
			(fontFace "Helvetica")
			(fontHeight 1.27)
			(strokeWidth 0.127)
		)
	)
	(patternDef "EVPASKC1A" (originalName "EVPASKC1A")
		(multiLayer
			(pad (padNum 1) (padStyleRef r200_200) (pt -3.000, 0.000) (rotation 0))
			(pad (padNum 2) (padStyleRef r200_200) (pt 3.000, 0.000) (rotation 0))
		)
		(layerContents (layerNumRef 18)
			(attr "RefDes" "RefDes" (pt -0.500, 0.000) (textStyleRef "Normal") (isVisible True))
		)
		(layerContents (layerNumRef 28)
			(line (pt -3 3.05) (pt 3 3.05) (width 0.025))
		)
		(layerContents (layerNumRef 28)
			(line (pt 3 3.05) (pt 3 -3.05) (width 0.025))
		)
		(layerContents (layerNumRef 28)
			(line (pt 3 -3.05) (pt -3 -3.05) (width 0.025))
		)
		(layerContents (layerNumRef 28)
			(line (pt -3 -3.05) (pt -3 3.05) (width 0.025))
		)
		(layerContents (layerNumRef Courtyard_Top)
			(line (pt -6 4.05) (pt 5 4.05) (width 0.1))
		)
		(layerContents (layerNumRef Courtyard_Top)
			(line (pt 5 4.05) (pt 5 -4.05) (width 0.1))
		)
		(layerContents (layerNumRef Courtyard_Top)
			(line (pt 5 -4.05) (pt -6 -4.05) (width 0.1))
		)
		(layerContents (layerNumRef Courtyard_Top)
			(line (pt -6 -4.05) (pt -6 4.05) (width 0.1))
		)
		(layerContents (layerNumRef 18)
			(line (pt -3 3.05) (pt 3 3.05) (width 0.1))
		)
		(layerContents (layerNumRef 18)
			(line (pt -3 -3.05) (pt 3 -3.05) (width 0.1))
		)
		(layerContents (layerNumRef 18)
			(line (pt -5 0) (pt -5 0) (width 0.2))
		)
		(layerContents (layerNumRef 18)
			(arc (pt -4.9, 0) (radius 0.1) (startAngle 180) (sweepAngle 180.0) (width 0.2))
		)
		(layerContents (layerNumRef 18)
			(line (pt -4.8 0) (pt -4.8 0) (width 0.2))
		)
		(layerContents (layerNumRef 18)
			(arc (pt -4.9, 0) (radius 0.1) (startAngle .0) (sweepAngle 180.0) (width 0.2))
		)
	)
	(symbolDef "EVP-ASKC1A" (originalName "EVP-ASKC1A")

		(pin (pinNum 1) (pt 0 mils 0 mils) (rotation 0) (pinLength 200 mils) (pinDisplay (dispPinName true)) (pinName (text (pt 230 mils -25 mils) (rotation 0]) (justify "Left") (textStyleRef "Normal"))
		))
		(pin (pinNum 2) (pt 0 mils -100 mils) (rotation 0) (pinLength 200 mils) (pinDisplay (dispPinName true)) (pinName (text (pt 230 mils -125 mils) (rotation 0]) (justify "Left") (textStyleRef "Normal"))
		))
		(line (pt 200 mils 100 mils) (pt 700 mils 100 mils) (width 6 mils))
		(line (pt 700 mils 100 mils) (pt 700 mils -200 mils) (width 6 mils))
		(line (pt 700 mils -200 mils) (pt 200 mils -200 mils) (width 6 mils))
		(line (pt 200 mils -200 mils) (pt 200 mils 100 mils) (width 6 mils))
		(attr "RefDes" "RefDes" (pt 750 mils 300 mils) (justify Left) (isVisible True) (textStyleRef "Normal"))
		(attr "Type" "Type" (pt 750 mils 200 mils) (justify Left) (isVisible True) (textStyleRef "Normal"))

	)
	(compDef "EVP-ASKC1A" (originalName "EVP-ASKC1A") (compHeader (numPins 2) (numParts 1) (refDesPrefix S)
		)
		(compPin "A1" (pinName "COM") (partNum 1) (symPinNum 1) (gateEq 0) (pinEq 0) (pinType Unknown))
		(compPin "B1" (pinName "NO") (partNum 1) (symPinNum 2) (gateEq 0) (pinEq 0) (pinType Unknown))
		(attachedSymbol (partNum 1) (altType Normal) (symbolName "EVP-ASKC1A"))
		(attachedPattern (patternNum 1) (patternName "EVPASKC1A")
			(numPads 2)
			(padPinMap
				(padNum 1) (compPinRef "A1")
				(padNum 2) (compPinRef "B1")
			)
		)
		(attr "Mouser Part Number" "667-EVP-ASKC1A")
		(attr "Mouser Price/Stock" "https://www.mouser.co.uk/ProductDetail/Panasonic/EVP-ASKC1A?qs=gTYE2QTfZfQol0eOIUHCMQ%3D%3D")
		(attr "Manufacturer_Name" "Panasonic")
		(attr "Manufacturer_Part_Number" "EVP-ASKC1A")
		(attr "Description" "Tactile Switches 3N 6x6.1x5mm 1.3mm Travel Black")
		(attr "<Hyperlink>" "https://industrial.panasonic.com/ac/cdn/e/control/switch/light-touch/catalog/sw_lt_eng_6s_long_2t.pdf")
		(attr "<Component Height>" "5.2")
		(attr "<STEP Filename>" "EVP-ASKC1A.stp")
		(attr "<STEP Offsets>" "X=0;Y=0;Z=0")
		(attr "<STEP Rotation>" "X=0;Y=0;Z=0")
	)

)
