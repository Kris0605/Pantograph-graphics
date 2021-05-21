#include "Drawables.h"

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {

	_shader.setDiffuseColor(_color)
		.setLightPosition(camera.cameraMatrix().transformPoint({ -30.0f, -3.0f, 50.0f }))
		.setTransformationMatrix(transformationMatrix)
		.setNormalMatrix(transformationMatrix.normalMatrix())
		.setProjectionMatrix(camera.projectionMatrix())
		.draw(_mesh);

}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
	_shader
		.setLightPosition(camera.cameraMatrix().transformPoint({ -30.0f, -3.0f, 50.0f }))
		.setTransformationMatrix(transformationMatrix)
		.setNormalMatrix(transformationMatrix.normalMatrix())
		.setProjectionMatrix(camera.projectionMatrix())
		.bindAmbientTexture(_diffuseTexture)
		.bindDiffuseTexture(_diffuseTexture)
		.draw(_mesh);
}