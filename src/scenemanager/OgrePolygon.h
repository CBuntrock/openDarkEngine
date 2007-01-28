/******************************************************************************
 *
 *    This file is part of openDarkEngine project
 *    Copyright (C) 2005-2006 openDarkEngine team
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA, or go to
 * http://www.gnu.org/copyleft/lesser.txt.
 *****************************************************************************/
 
#ifndef _OgrePolygon_H__
#define _OgrePolygon_H__

#include "OgreVector3.h"
#include "OgrePlane.h"
#include <iostream>

namespace Ogre {

	/** A vector of Vertices used for Portal shape definition */
	typedef std::vector< Vector3 > PolygonPoints;
	
	/** @brief A Polygon class used as a base for the Portal class as well as other things
	*/
	class Polygon {
		friend class DarkSceneManager;
			
		protected:
			/** A vector containing the Vector3 values - Portal/portal edge vertices */
			PolygonPoints 	*mPoints;
	
			/** The plane on which the portal lies */
			Plane		mPlane;
			
		
		public:
			/** Default constructor. Defines an empty geometry which will lie on the specified plane
			* @param plane Polygon's plane (Normal direction does matter) */
			Polygon(Plane plane);
		
			~Polygon();
			
			/** copy ctor */
			Polygon(Polygon *src);
			
			void addPoint(float x, float y, float z);
			
			void addPoint(Vector3 a);
			
			const PolygonPoints& getPoints();
		
			int getPointCount();
			
			const Plane& getPlane();
			
			void setPlane(Plane plane);
			
			/** get the number of polygons's vertices outside a given plane
			* @return the count of points which lie outside (negative distance from the plane)
			*/
			unsigned int getOutCount(Plane &plane);
			
			/** Clips the poly using a plane (and replaces instances vertices after success)
			* @return number of vertices in the new poly
			*/
			int clipByPlane(const Plane &plane, bool &didClip);
			
			/** Optimizes the portal. Removes unneeded, unnecessary vertices which slow down the visibility evaluation.
			* @return int Number of vertices removed */
			int optimize();
			
			/** Returns true if the ray hits the polygon */
			bool isHitBy(const Ray& ray) const;
			
			/** Returns true if the sphere is totaly enclosed by the portal on the intersection with the portal's plane 
			* @param pos Sphere center
			* @param radius Sphere radius
			* @param distance Sphere center to polygon's plane distance
			*/
			bool enclosesSphere(const Vector3& pos, const Real& radius, const Real& distance) const;
	};

}

#endif
