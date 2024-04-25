
// (c) 2010 Geom Software Bernhard Kornberger, Graz/Austria. All rights reserved.
//
// This file is part of the Fade2D library. The licensee mentioned below may use 
// this license file in accordance with the commercial license agreement. This 
// license file is personalized. DO NOT SHARE IT. 

// This software is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
// THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Please contact the author if any conditions of this licensing are not clear 
// to you.
// 
// Geom Software e.U.
// Dipl.-Ing. Dr. Bernhard Kornberger
// Raiffeisenstrasse 19
// A-8010 Graz / Austria
// bkorn@geom.at 
// 
// https://www.geom.at/fade2d/html/
// https://www.geom.at/fade25d/html/


#pragma once 
#include "Fade_2D.h"

namespace{
#if GEOM_PSEUDO3D==GEOM_TRUE
	namespace GEOM_LIC=GEOM_FADE25D;
#elif GEOM_PSEUDO3D==GEOM_FALSE
	namespace GEOM_LIC=GEOM_FADE2D;
#endif

struct License
{
License()
{
	GEOM_LIC::setLic(

// (c) 2010 Geom Software Bernhard Kornberger, Graz/Austria. All rights reserved.
//
// This file is part of the Fade2D library. The licensee mentioned below may use
// this license file for evaluation purposes during the agreed period
// Commercial use requires a valid commercial license, this applies also to
// inhouse usage. This license file is personalized, DO NOT SHARE IT.

// This software is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
// THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Please contact the author if any conditions of this licensing are not clear 
// to you.
// 
// Geom Software e.U.
// Dipl.-Ing. Dr. Bernhard Kornberger
// Raiffeisenstrasse 19
// A-8010 Graz / Austria
// bkorn@geom.at 
// 
// https://www.geom.at/fade2d/html/
// https://www.geom.at/fade25d/html/


#pragma once 
#include "Fade_2D.h"

namespace{
#if GEOM_PSEUDO3D==GEOM_TRUE
	namespace GEOM_LIC=GEOM_FADE25D;
#elif GEOM_PSEUDO3D==GEOM_FALSE
	namespace GEOM_LIC=GEOM_FADE2D;
#endif

struct License
{
License()
{
	GEOM_LIC::setLic(
		"Andres Mason;Carnegie Mellon University;5000 Forbes Avenue;Pittsburgh, PA 15213;;Research license, valid for personal research until 4/2027; Not valid for use in commercial software",
		"[LicType,eval],[2D,max],[25D,0],[MeshGen,max],[SegCheck,0],[CutFill,0],[Voronoi,0]",
		"[LF:F/C]",
		"8710fbbd",
		"2e600fdc");
	}
};
License lic;
}
