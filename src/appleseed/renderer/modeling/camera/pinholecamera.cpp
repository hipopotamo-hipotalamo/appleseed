
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "pinholecamera.h"

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/kernel/shading/shadingray.h"
#include "renderer/modeling/camera/camera.h"
#include "renderer/utility/transformsequence.h"

// appleseed.foundation headers.
#include "foundation/math/matrix.h"
#include "foundation/math/transform.h"
#include "foundation/math/vector.h"
#include "foundation/platform/compiler.h"
#include "foundation/utility/containers/specializedarrays.h"
#include "foundation/utility/autoreleaseptr.h"

// Standard headers.
#include <cassert>

// Forward declarations.
namespace renderer  { class Project; }

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    //
    // Pinhole camera.
    //

    const char* Model = "pinhole_camera";

    class PinholeCamera
      : public Camera
    {
      public:
        PinholeCamera(
            const char*             name,
            const ParamArray&       params)
          : Camera(name, params)
        {
        }

        virtual void release() OVERRIDE
        {
            delete this;
        }

        virtual const char* get_model() const OVERRIDE
        {
            return Model;
        }

        virtual bool on_frame_begin(const Project& project) OVERRIDE
        {
            if (!Camera::on_frame_begin(project))
                return false;

            m_film_dimensions = get_film_dimensions();
            m_rcp_film_width = 1.0 / m_film_dimensions[0];
            m_rcp_film_height = 1.0 / m_film_dimensions[1];

            // Precompute the rays origin in world space if the camera is static.
            if (m_transform_sequence.size() <= 1)
                m_ray_org = m_transform_sequence.evaluate(0.0).point_to_parent(Vector3d(0.0));

            return true;
        }

        virtual void generate_ray(
            SamplingContext&        sampling_context,
            const Vector2d&         point,
            ShadingRay&             ray) const OVERRIDE
        {
            // Initialize the ray.
            initialize_ray(sampling_context, ray);

            // Transform the film point from NDC to camera space.
            const Vector3d target(
                (point.x - 0.5) * m_film_dimensions[0],
                (0.5 - point.y) * m_film_dimensions[1],
                -m_focal_length);

            // Compute the origin and direction of the ray.
            Transformd tmp;
            const Transformd& transform = m_transform_sequence.evaluate(ray.m_time, tmp);
            ray.m_org =
                m_transform_sequence.size() <= 1
                    ? m_ray_org
                    : transform.get_local_to_parent().extract_translation();
            ray.m_dir = transform.vector_to_parent(target);
        }

        virtual Vector2d project(const Vector3d& point) const OVERRIDE
        {
            const double k = -m_focal_length / point.z;
            const double x = 0.5 + (point.x * k * m_rcp_film_width);
            const double y = 0.5 - (point.y * k * m_rcp_film_height);
            return Vector2d(x, y);
        }

      private:
        // Parameters.
        Vector2d    m_film_dimensions;      // film dimensions, in meters

        // Precomputed values.
        double      m_rcp_film_width;       // film width reciprocal in camera space
        double      m_rcp_film_height;      // film height reciprocal in camera space
        Vector3d    m_ray_org;              // origin of the rays in world space
    };
}


//
// PinholeCameraFactory class implementation.
//

const char* PinholeCameraFactory::get_model() const
{
    return Model;
}

const char* PinholeCameraFactory::get_human_readable_model() const
{
    return "Pinhole Camera";
}

DictionaryArray PinholeCameraFactory::get_input_metadata() const
{
    DictionaryArray metadata = CameraFactory::get_input_metadata();

    return metadata;
}

auto_release_ptr<Camera> PinholeCameraFactory::create(
    const char*         name,
    const ParamArray&   params) const
{
    return auto_release_ptr<Camera>(new PinholeCamera(name, params));
}

}   // namespace renderer
