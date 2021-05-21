#include "Graphics.h"
#include "Drawables.h"

#undef far
#undef near

ThesisGraphics::ThesisGraphics(const Arguments& arguments, const Configuration& configuration) : Platform::Application{ arguments, configuration } {
	ExitSimuSerial = false;
	std::cout << "SERIAL: " << arguments.argv[0] << std::endl;
	// Setting port for serial communication
	_serial.setPort(arguments.argv[0]);
	_serial.openOutputFile();

	/* Base object, parent of all (for easy manipulation) */
	_manipulator.setParent(&_scene);
	_endEffector.setParent(&_manipulator);
	_ball.setParent(&_manipulator);
	_spinner.setParent(&_manipulator);
	_opponent.setParent(&_manipulator);

	/* Setup renderer and shader defaults */
	GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
	GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

	_coloredShader
		.setAmbientColor(0x111111_rgbf)
		.setSpecularColor(0xffffff_rgbf)
		.setShininess(80.0f);
	_texturedShader
		.setAmbientColor(0x111111_rgbf)
		.setSpecularColor(0x111111_rgbf)
		.setShininess(80.0f);

	/* Load a scene importer plugin */
	PluginManager::Manager<Trade::AbstractImporter> manager;
	Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnySceneImporter");
	if (!importer) std::exit(1);

	/* Load file containing the scene */
	switch (SimIndex)
	{
	case 0: { /*Billiard*/
		if (!importer->openFile(Utility::Directory::join("models", "Billiard.glb")))
			std::exit(4);
		_serial.write('1');
		break;
	}
	case 1: { /*Airhockey*/
		if (!importer->openFile(Utility::Directory::join("models", "Airhockey.glb")))
			std::exit(4);
		_serial.write('2');
		_manipulator.rotateX(Deg{ -50 });
		break;
	}
	case 2: { /*Spinner*/

		if (!importer->openFile(Utility::Directory::join("models", "Spinner.glb")))
			std::exit(4);
		_serial.write('3');
		_previousSpinnerScale = 1.0f;
		_previousSpinnerAlpha = 0.0f;
		_manipulator.rotateX(Deg{ -50 });
		break;
	}
	case 3: { /*Labyrinth*/
		if (!importer->openFile(Utility::Directory::join("models", "Labyrinth.glb")))
			std::exit(4);
		_serial.write('4');
		break;
	}
	case 4: { /*Coffee*/
		if (!importer->openFile(Utility::Directory::join("models", "Coffee.glb")))
			std::exit(4);
		_serial.write('5');
		_previousSpinnerScale = 1.0f;
		_previousSpinnerAlpha = 0.0f;
		_manipulator.rotateX(Deg{ -50 });
		break;
	}
	default:
		break;
	}

	/* Load all textures. Textures that fail to load will be NullOpt. */
	_textures = Containers::Array<Containers::Optional<GL::Texture2D>>{ importer->textureCount() };
	for (UnsignedInt i = 0; i != importer->textureCount(); ++i) {
		Debug{} << "Importing texture" << i << importer->textureName(i);

		Containers::Optional<Trade::TextureData> textureData = importer->texture(i);
		if (!textureData || textureData->type() != Trade::TextureData::Type::Texture2D) {
			Warning{} << "Cannot load texture properties, skipping";
			continue;
		}

		Debug{} << "Importing image" << textureData->image() << importer->image2DName(textureData->image());

		Containers::Optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());
		GL::TextureFormat format;
		if (imageData && imageData->format() == PixelFormat::RGB8Unorm) {
			format = GL::TextureFormat::RGB8;
		}
		else if (imageData && imageData->format() == PixelFormat::RGBA8Unorm) {
			format = GL::TextureFormat::RGBA8;
		}
		else {
			Warning{} << "Cannot load texture image, skipping";
			continue;
		}

		/* Configure the texture */
		GL::Texture2D texture;
		texture
			.setMagnificationFilter(textureData->magnificationFilter())
			.setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
			.setWrapping(textureData->wrapping().xy())
			.setStorage(Math::log2(imageData->size().max()) + 1, format, imageData->size())
			.setSubImage(0, {}, *imageData)
			.generateMipmap();

		_textures[i] = std::move(texture);
	}

	/* Load all materials. Materials that fail to load will be NullOpt. The
	   data will be stored directly in objects later, so save them only
	   temporarily. */
	Containers::Array<Containers::Optional<Trade::PhongMaterialData>> materials{ importer->materialCount() };
	for (UnsignedInt i = 0; i != importer->materialCount(); ++i) {
		Debug{} << "Importing material" << i << importer->materialName(i);

		Containers::Pointer<Trade::AbstractMaterialData> materialData = importer->material(i);
		if (!materialData || materialData->type() != Trade::MaterialType::Phong) {
			Warning{} << "Cannot load material, skipping";
			continue;
		}

		materials[i] = std::move(static_cast<Trade::PhongMaterialData&>(*materialData));
	}

	/* Load all meshes. Meshes that fail to load will be NullOpt. */
	_meshes = Containers::Array<Containers::Optional<GL::Mesh>>{ importer->meshCount() };
	for (UnsignedInt i = 0; i != importer->meshCount(); ++i) {
		Debug{} << "Importing mesh" << i << importer->meshName(i);

		Containers::Optional<Trade::MeshData> meshData = importer->mesh(i);
		if (!meshData || !meshData->hasAttribute(Trade::MeshAttribute::Normal) || meshData->primitive() != MeshPrimitive::Triangles) {
			Warning{} << "Cannot load the mesh, skipping";
			continue;
		}

		/* Compile the mesh */
		_meshes[i] = MeshTools::compile(*meshData);
	}

	/* Load the scene */
	if (importer->defaultScene() != -1) {
		Debug{} << "Adding default scene" << importer->sceneName(importer->defaultScene());

		Containers::Optional<Trade::SceneData> sceneData = importer->scene(importer->defaultScene());
		if (!sceneData) {
			Error{} << "Cannot load scene, exiting";
			return;
		}

		/* Recursively add all children */
		for (UnsignedInt objectId : sceneData->children3D())
			addObject(*importer, materials, _manipulator, objectId);

		/* The format has no scene support, display just the first loaded mesh with
		   a default material and be done with it */
	}
	else if (!_meshes.empty() && _meshes[0])
		new ColoredDrawable{ _manipulator, _coloredShader, *_meshes[0], 0xffffff00_rgbaf, _drawables };

	Containers::Optional<Trade::CameraData> cameraData = importer->camera(0);

	/* Every scene needs a camera */

	_cameraObject
		.setParent(&_scene)
		.translate(Vector3::zAxis(0.5f));

	(*(_camera = new SceneGraph::Camera3D{ _cameraObject }))
		.setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
		.setProjectionMatrix(Matrix4::perspectiveProjection(cameraData->fov(), cameraData->aspectRatio(), cameraData->near(), cameraData->far()))
		.setViewport(GL::defaultFramebuffer.viewport().size());

	_ball.translate(Vector3(Vector2(0.065f, 0.0f) - _previousBallPosition, 0.0f));
	_previousBallPosition = Vector2(0.065f, 0.0f);
}

void ThesisGraphics::addObject(Trade::AbstractImporter& importer, Containers::ArrayView<const Containers::Optional<Trade::PhongMaterialData>> materials, Object3D& parent, UnsignedInt i) {
	Debug{} << "Importing object" << i << importer.object3DName(i);
	Containers::Pointer<Trade::ObjectData3D> objectData = importer.object3D(i);
	if (!objectData) {
		Error{} << "Cannot import object, skipping";
		return;
	}

	bool isBall = importer.object3DName(i) == "node1";
	bool isInEndEffector = importer.object3DName(i) == "node2" || importer.object3DName(i) == "node3" || importer.object3DName(i) == "node4";
	bool isSpinner = importer.object3DName(i) == "node5" || importer.object3DName(i) == "node6";
	bool isOpponent = importer.object3DName(i) == "node10";
	
	/* Add the object to the scene and set its transformation */
	auto* object = isInEndEffector ? new Object3D{ &_endEffector } : (isBall ? new Object3D{ &_ball } : (isSpinner ? new Object3D{ &_spinner } : (isOpponent ? new Object3D{ &_opponent } : new Object3D{ &parent })));

	object->setTransformation(objectData->transformation());
	object->rotateX(Deg{ 90 });

	/* Add a drawable if the object has a mesh and the mesh is loaded */
	if (objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh && objectData->instance() != -1 && _meshes[objectData->instance()]) {
		const Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();

		/* Material not available / not loaded, use a default material */
		if (materialId == -1 || !materials[materialId]) {
			new ColoredDrawable{ *object, _coloredShader, *_meshes[objectData->instance()], 0xffffff_rgbf, _drawables };
			/* Textured material. If the texture failed to load, again just use a
			   default colored material. */
		}
		else if (materials[materialId]->flags() & Trade::PhongMaterialData::Flag::DiffuseTexture) {
			Containers::Optional<GL::Texture2D>& diffuseTexture = _textures[materials[materialId]->diffuseTexture()];
			if (diffuseTexture) {
				new TexturedDrawable{ *object, _texturedShader, *_meshes[objectData->instance()], *diffuseTexture, _drawables };
			}
			else
				new ColoredDrawable{ *object, _coloredShader, *_meshes[objectData->instance()], 0xffffff_rgbf, _drawables };
			/* Color-only material */
		}
		else {
			new ColoredDrawable{ *object, _coloredShader, *_meshes[objectData->instance()], materials[materialId]->diffuseColor(), _drawables };
		}
	}

	/* Recursively add children */
	for (std::size_t id : objectData->children())
		addObject(importer, materials, *object, id);
}

void ThesisGraphics::drawEvent() {
	if (ExitSimu || ExitSimuSerial) {
		ThesisGraphics::exit(0);
	}
	GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
	_camera->draw(_drawables);
	swapBuffers();
}

void ThesisGraphics::viewportEvent(ViewportEvent& event) {
	GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });
	_camera->setViewport(event.windowSize());
}

void ThesisGraphics::mousePressEvent(MouseEvent& event) {
	if (event.button() == MouseEvent::Button::Left)
		_previousPosition = positionOnSphere(event.position());
}

void ThesisGraphics::mouseReleaseEvent(MouseEvent& event) {
	if (event.button() == MouseEvent::Button::Left)
		_previousPosition = Vector3();
}

void ThesisGraphics::mouseScrollEvent(MouseScrollEvent& event) {
	if (!event.offset().y()) return;
	// Distance to origin
	const Float distance = _cameraObject.transformation().translation().z();
	// Move 15% of the distance back or forward
	_cameraObject.translate(Vector3::zAxis(
		distance * (1.0f - (event.offset().y() > 0 ? 1 / 0.85f : 0.85f))));
	redraw();
}

Vector3 ThesisGraphics::positionOnSphere(const Vector2i& position) const {
	const Vector2 positionNormalized = Vector2{ position } / Vector2{ _camera->viewport() } - Vector2{ 0.5f };
	const Float length = positionNormalized.length();
	const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
	return (result * Vector3::yScale(-1.0f)).normalized();
}

void ThesisGraphics::mouseMoveEvent(MouseMoveEvent& event) {
	if (!(event.buttons() & MouseMoveEvent::Button::Left)) return;
	const Vector3 currentPosition = positionOnSphere(event.position());
	const Vector3 axis = Math::cross(_previousPosition, currentPosition);
	if (_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;
	_manipulator.rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());
	_previousPosition = currentPosition;
	redraw();
}

// Updating positions of objects
void ThesisGraphics::tickEvent() {
	_serial.read();
	_endEffector.translate(Vector3(Vector2(_serial.effectorX, _serial.effectorY) - _previousEffectorPosition, 0.0f));
	_previousEffectorPosition = Vector2(_serial.effectorX, _serial.effectorY);
	switch (SimIndex)
	{
	case 0: { /*Billiard*/
		_ball.translate(Vector3(Vector2(_serial.ballX, _serial.ballY) - _previousBallPosition, 0.0f));
		_previousBallPosition = Vector2(_serial.ballX, _serial.ballY);
		break;
	}
	case 1: { /*Airhockey*/
		_ball.translate(Vector3(Vector2(_serial.ballX, _serial.ballY) - _previousBallPosition, 0.0f));
		_previousBallPosition = Vector2(_serial.ballX, _serial.ballY);
		_opponent.translate(Vector3(Vector2(_serial.opponentX, _serial.opponentY) - _previousOpponentPosition, 0.0f));
		_previousOpponentPosition = Vector2(_serial.opponentX, _serial.opponentY);
		break;
	}
	case 2: { /*Spinner*/
		_spinner.rotateZ(Rad{ _serial.spinnerAlpha - _previousSpinnerAlpha });
		_previousSpinnerAlpha = _serial.spinnerAlpha;
		break;
	}
	case 3: { /*Labyrinth*/
		break;
	}
	case 4: { /*Coffee*/
		_spinner.rotateZ(Rad{ _serial.spinnerAlpha - _previousSpinnerAlpha });
		/*203.8736=R^2/(2*g*H*dt^2)*10 R=0,1 paraboloid sugara, dt=0.005 szimulációban használt dt, g=9,81, H=0,1 paraboloid magassága*/
		_spinnerScale = 203.8736f * pow((_serial.spinnerAlpha - _previousSpinnerAlpha), 2) + 0.00001f;
		_spinner.scale(Vector3(1.0f, 1.0f, (_spinnerScale / _previousSpinnerScale)));

		_previousSpinnerAlpha = _serial.spinnerAlpha;
		_previousSpinnerScale = _spinnerScale;
		break;
	}
	default:
		break;
	}
	redraw();
}

ThesisGraphics::~ThesisGraphics() {
	enableWindow();
	if (ExitSimuSerial) {
		SelectComPort();
	}
	else {
		_serial.write('0');
	}
}