#include <vector>
#include <LinearMath/btIDebugDraw.h>
#include <LinearMath/btVector3.h>
#include <LinearMath/btQuaternion.h>
#include <LinearMath/btTransform.h>

class MyDebugDrawer : public btIDebugDraw {
private:
    // Storage for triangulated rectangles
    std::vector<btVector3> m_vertices;
    std::vector<btVector3> m_colors;
    std::vector<int> m_indices;
    
    // Rectangle width
    btScalar m_rectWidth;
    
public:
    MyDebugDrawer(btScalar rectWidth = 0.05f) : m_rectWidth(rectWidth) {}
    
    // Convert a line segment to a double-sided rectangle represented as two triangles
    virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {
        // Direction vector of the line
        btVector3 lineDir = (to - from).normalized();
        
        // Find perpendicular vectors to create the rectangle
        // We need to find a consistent "up" vector to avoid flickering
        btVector3 up(0.0f, 1.0f, 0.0f);
        if (fabs(lineDir.dot(up)) > 0.99f) {
            // If lineDir is nearly parallel to up, use a different reference
            up.setValue(1.0f, 0.0f, 0.0f);
        }
        
        // Cross products to get perpendicular vectors
        btVector3 right = lineDir.cross(up).normalized();
        btVector3 perpUp = right.cross(lineDir).normalized();
        
        // Scale by rectangle width
        right *= m_rectWidth * 0.5f;
        perpUp *= m_rectWidth * 0.5f;
        
        // Calculate the four corners of the rectangle
        btVector3 corners[4];
        corners[0] = from + right - perpUp;  // Bottom right at 'from' end
        corners[1] = from - right - perpUp;  // Bottom left at 'from' end
        corners[2] = to - right + perpUp;    // Top left at 'to' end
        corners[3] = to + right + perpUp;    // Top right at 'to' end
        
        // Record start index for this rectangle
        int baseIndex = m_vertices.size();
        
        // Add the vertices to our list
        for (int i = 0; i < 4; i++) {
            m_vertices.push_back(corners[i]);
            m_colors.push_back(color);
        }
        
        // First triangle: Bottom-right, bottom-left, top-left (0, 1, 2)
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 1);
        m_indices.push_back(baseIndex + 2);
        
        // Second triangle: Bottom-right, top-left, top-right (0, 2, 3)
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 3);
        
        // Add the same triangles but with reversed winding order for double-sided rendering
        // Third triangle (back): Bottom-right, top-left, bottom-left (0, 2, 1)
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 2);
        m_indices.push_back(baseIndex + 1);
        
        // Fourth triangle (back): Bottom-right, top-right, top-left (0, 3, 2)
        m_indices.push_back(baseIndex + 0);
        m_indices.push_back(baseIndex + 3);
        m_indices.push_back(baseIndex + 2);
    }
    
    // Draw contact points as small rectangles in different directions
    virtual void drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, 
                                 btScalar distance, int lifeTime, const btVector3& color) override {
        btVector3 to = pointOnB + normalOnB * distance;
        drawLine(pointOnB, to, color);
        
        // Add perpendicular lines to make the contact point more visible
        btScalar contactMarkerSize = m_rectWidth * 2.0f;
        btVector3 tangent1, tangent2;
        
        // Find perpendicular directions
        btPlaneSpace1(normalOnB, tangent1, tangent2);
        
        // Draw a small cross at the contact point
        drawLine(pointOnB - tangent1 * contactMarkerSize, 
                 pointOnB + tangent1 * contactMarkerSize, color);
        drawLine(pointOnB - tangent2 * contactMarkerSize, 
                 pointOnB + tangent2 * contactMarkerSize, color);
    }
    
    // Required interface methods
    virtual void reportErrorWarning(const char* warningString) override {}
    virtual void draw3dText(const btVector3& location, const char* textString) override {}
    
    // Configuration for what gets drawn
    virtual int getDebugMode() const override {
        return DBG_DrawWireframe | DBG_DrawContactPoints | DBG_DrawConstraints | DBG_DrawConstraintLimits;
    }
    
    virtual void setDebugMode(int debugMode) override {}
    
    // Access methods for the collected geometry
    const std::vector<btVector3>& getVertices() const { return m_vertices; }
    const std::vector<btVector3>& getColors() const { return m_colors; }
    const std::vector<int>& getIndices() const { return m_indices; }
    
    // Set the width of the rectangles
    void setRectangleWidth(btScalar width) { m_rectWidth = width; }
    btScalar getRectangleWidth() const { return m_rectWidth; }
    
    // Clear data after rendering
    void clear() {
        m_vertices.clear();
        m_colors.clear();
        m_indices.clear();
    }
};