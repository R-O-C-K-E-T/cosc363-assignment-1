#pragma once

#include <glm/glm.hpp>

#include "vec12.h"

#include <cmath>
#include <cstddef>
#include <assert.h>

class vec12 {
	public:
		explicit constexpr vec12(float a, float b, float c, float d, float e, float f, float g, float h, float i, float j, float k, float l) : d{a,b,c,d,e,f,g,h,i,j,k,l} {}
		explicit constexpr vec12(float v) : d{v,v,v,v,v,v,v,v,v,v,v,v} {}
		explicit constexpr vec12() : vec12(0.0f) {}
		explicit constexpr vec12(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) : d{a.x,a.y,a.z,b.x,b.y,b.z,c.x,c.y,c.z,d.x,d.y,d.z} {}
		

		const constexpr float& operator[](size_t i) const {
			assert(i >= 0 && i < 12);
			return d[i];
		}

		constexpr float& operator[](size_t i) {
			assert(i >= 0 && i < 12);
			return d[i];
		}

		vec12 constexpr operator-() const {
			auto result = vec12();
			for (size_t i=0; i<12; i++) result.d[i] = -d[i];
			return result;
		}

		float constexpr length2() const {
			float result = 0.0f;
			for (size_t i = 0; i<12; i++) result += d[i]*d[i];
			return result;
		}

		float length() const {
			return glm::sqrt(length2());
		}

		vec12 normalised() const {
			return *this * (1.0f / length());
		}

		float constexpr dot(const vec12& other) const {
			float result = 0.0f;
			for (size_t i=0; i<12; i++) result += d[i] * other.d[i];
			return result;
		}

		#define SIMPLE_OP(OP) vec12 constexpr operator OP (const vec12& other) const {\
			auto result = vec12();\
			for (size_t i=0; i<12; i++) result.d[i] = d[i] OP other.d[i];\
			return result;\
		}\
		vec12 constexpr operator OP (const float factor) const {\
			auto result = vec12();\
			for (size_t i=0; i<12; i++) result.d[i] = d[i] OP factor;\
			return result;\
		}\
		friend vec12 constexpr operator OP (const float factor, const vec12& val) {\
			auto result = vec12();\
			for (size_t i=0; i<12; i++) result.d[i] = factor OP val.d[i];\
			return result;\
		}\
		void constexpr operator OP##= (const vec12& val) {\
			for (size_t i=0; i<12; i++) d[i] OP##= val.d[i];\
		}\
		void constexpr operator OP##= (const float factor) {\
			for (size_t i=0; i<12; i++) d[i] OP##= factor;\
		}

		SIMPLE_OP(+)
		SIMPLE_OP(-)
		SIMPLE_OP(*)
		SIMPLE_OP(/)

		#undef SIMPLE_OP
	private:
		float d[12];
};