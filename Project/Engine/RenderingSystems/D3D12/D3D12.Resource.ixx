// [Difference between resources and resource views]
// Resource: data to be written/read, e.g. a texture or a buffer
// View: data formatted in certain ways so as to be interpreted properly by the graphic drivers
// Reference: https://stackoverflow.com/questions/56821782/what-is-the-difference-between-a-resource-and-a-resource-view

//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////
//////	//////	//////	//////	//////	//////

export module Lumina.D3D12 : Resource;

export import : Resource.Common;

export import : Resource.Buffer;
export import : Resource.Texture2D;