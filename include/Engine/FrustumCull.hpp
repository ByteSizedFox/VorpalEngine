#pragma once
// https://gist.github.com/podgorskiy/e698d18879588ada9014768e3e82a644

#include <glm/matrix.hpp>

class Frustum {
public:
	Frustum();

	// m = ProjectionMatrix * ViewMatrix 
	Frustum(glm::mat4 m);
	void update(glm::mat4 m);

	// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
	bool IsBoxVisible(const glm::vec3& minp, const glm::vec3& maxp);

private:
	enum Planes {
		Left = 0,
		Right,
		Bottom,
		Top,
		Near,
		Far,
		Count,
		Combinations = Count * (Count - 1) / 2
	};

	template<Planes i, Planes j>
	struct ij2k {
		enum { k = i * (9 - i) / 2 + j - 1 };
	};

	template<Planes a, Planes b, Planes c>
	glm::vec3 intersection(const glm::vec3* crosses);
	glm::vec4   m_planes[Count];
	glm::vec3   m_points[8];
};