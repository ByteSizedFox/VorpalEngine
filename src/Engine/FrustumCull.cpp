#include "Engine/FrustumCull.hpp"

// FIXME: for some reason using frustum culling with mingw causes segfaults

Frustum::Frustum() {
	// nothing
}

Frustum::Frustum(glm::mat4 m) {
	update(m);
}

void Frustum::update(glm::mat4 m) {
	m = glm::transpose(m);
	m_planes[Left]   = m[3] + m[0];
	m_planes[Right]  = m[3] - m[0];
	m_planes[Bottom] = m[3] + m[1];
	m_planes[Top]    = m[3] - m[1];
	m_planes[Near]   = m[3] + m[2];
	m_planes[Far]    = m[3] - m[2];

	glm::vec3 crosses[Combinations] = {
		glm::cross(glm::vec3(m_planes[Left]),   glm::vec3(m_planes[Right])),
		glm::cross(glm::vec3(m_planes[Left]),   glm::vec3(m_planes[Bottom])),
		glm::cross(glm::vec3(m_planes[Left]),   glm::vec3(m_planes[Top])),
		glm::cross(glm::vec3(m_planes[Left]),   glm::vec3(m_planes[Near])),
		glm::cross(glm::vec3(m_planes[Left]),   glm::vec3(m_planes[Far])),
		glm::cross(glm::vec3(m_planes[Right]),  glm::vec3(m_planes[Bottom])),
		glm::cross(glm::vec3(m_planes[Right]),  glm::vec3(m_planes[Top])),
		glm::cross(glm::vec3(m_planes[Right]),  glm::vec3(m_planes[Near])),
		glm::cross(glm::vec3(m_planes[Right]),  glm::vec3(m_planes[Far])),
		glm::cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Top])),
		glm::cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Near])),
		glm::cross(glm::vec3(m_planes[Bottom]), glm::vec3(m_planes[Far])),
		glm::cross(glm::vec3(m_planes[Top]),    glm::vec3(m_planes[Near])),
		glm::cross(glm::vec3(m_planes[Top]),    glm::vec3(m_planes[Far])),
		glm::cross(glm::vec3(m_planes[Near]),   glm::vec3(m_planes[Far]))
	};

	m_points[0] = intersection<Left,  Bottom, Near>(crosses);
	m_points[1] = intersection<Left,  Top,    Near>(crosses);
	m_points[2] = intersection<Right, Bottom, Near>(crosses);
	m_points[3] = intersection<Right, Top,    Near>(crosses);
	m_points[4] = intersection<Left,  Bottom, Far>(crosses);
	m_points[5] = intersection<Left,  Top,    Far>(crosses);
	m_points[6] = intersection<Right, Bottom, Far>(crosses);
	m_points[7] = intersection<Right, Top,    Far>(crosses);
}

// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
bool Frustum::IsBoxVisible(const glm::vec3& minp, const glm::vec3& maxp) {
	// check box outside/inside of frustum
	for (int i = 0; i < Count; i++) {
		if ((glm::dot(m_planes[i], glm::vec4(minp.x, minp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, minp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, maxp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, maxp.y, minp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, minp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, minp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(minp.x, maxp.y, maxp.z, 1.0f)) < 0.0) &&
			(glm::dot(m_planes[i], glm::vec4(maxp.x, maxp.y, maxp.z, 1.0f)) < 0.0))
		{
			return false;
		}
	}

	// check frustum outside/inside box
    int out1 = 0;
    int out2 = 0;
    int out3 = 0;
    int out4 = 0;
    int out5 = 0;
    int out6 = 0;
    for (int i = 0; i < 8; i++) {
        out1 += ((m_points[i].x > maxp.x) ? 1 : 0);
        if (out1 == 8) return false;

        out2 += ((m_points[i].x < minp.x) ? 1 : 0);
        if (out2 == 8) return false;

        out3 += ((m_points[i].y > maxp.y) ? 1 : 0);
        if (out3 == 8) return false;

        out4 += ((m_points[i].y < minp.y) ? 1 : 0);
        if (out4 == 8) return false;

        out5 += ((m_points[i].z > maxp.z) ? 1 : 0);
        if (out5 == 8) return false;

        out6 += ((m_points[i].z < minp.z) ? 1 : 0);
        if (out6 == 8) return false;
    }

	return true;
}

template<Frustum::Planes a, Frustum::Planes b, Frustum::Planes c>
glm::vec3 Frustum::intersection(const glm::vec3* crosses) {
	float D = glm::dot(glm::vec3(m_planes[a]), crosses[ij2k<b, c>::k]);
	glm::vec3 res = glm::mat3(crosses[ij2k<b, c>::k], -crosses[ij2k<a, c>::k], crosses[ij2k<a, b>::k]) *
		glm::vec3(m_planes[a].w, m_planes[b].w, m_planes[c].w);
	return res * (-1.0f / D);
}