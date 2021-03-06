#include "material_viewer.h"

#include "core/resource_loader.h"
#include "core/resource_files/text_resource_file.h"

Ref<ResourceFile> MaterialViewer::load(const FilePath& filePath)
{
	m_OpenedMaterial = ResourceLoader::CreateMaterialResourceFile(filePath.generic_string());
	return m_OpenedMaterial;
}

void MaterialViewer::unload()
{
	m_OpenedMaterial.reset();
}

void MaterialViewer::draw(float deltaMilliseconds)
{
	m_OpenedMaterial->draw();
}
