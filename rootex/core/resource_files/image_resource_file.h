#pragma once

#include "resource_file.h"
#include "renderer/texture.h"

/// Representation of an image file. Supports BMP, JPEG, PNG, TIFF, GIF, HD Photo, or other WIC supported file containers
class ImageResourceFile : public ResourceFile
{
	Ref<Texture> m_ImageTexture;

	explicit ImageResourceFile(const FilePath& path);

	friend class ResourceLoader;

public:
	explicit ImageResourceFile(const ImageResourceFile&) = delete;
	explicit ImageResourceFile(const ImageResourceFile&&) = delete;
	~ImageResourceFile() = default;

	void reimport() override;

	const Ref<Texture> getTexture() { return m_ImageTexture; }
	unsigned int getWidth() const { return m_ImageTexture->getWidth(); }
	unsigned int getHeight() const { return m_ImageTexture->getHeight(); }
};
