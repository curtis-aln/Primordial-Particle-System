#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class DensityGridSmoother
{
public:
    DensityGridSmoother() = default;

    void init(int W, int H)
    {
        m_W = W; m_H = H;

        m_tex.resize({ (unsigned)W, (unsigned)H });
        m_tex.setSmooth(false);

        m_pingpong[0].resize({ (unsigned)W, (unsigned)H });
        m_pingpong[1].resize({ (unsigned)W, (unsigned)H });
        m_pingpong[0].setSmooth(false);
        m_pingpong[1].setSmooth(false);

        // load shaders
        if (!m_box_shader.loadFromFile("box_blur.frag", sf::Shader::Type::Fragment))
            throw std::runtime_error("box_blur.frag failed to load");

        if (!m_gaussian_shader.loadFromFile("gaussian_blur.frag", sf::Shader::Type::Fragment))
            throw std::runtime_error("gaussian_blur.frag failed to load");
    }

    // Call instead of smooth_box() / smooth_gaussian()
    // Uploads m_D, runs shader passes, writes result back to m_D
    void smooth_box(std::vector<float>& m_D, int cascade_passes)
    {
        upload(m_D);
        for (int i = 0; i < cascade_passes; i++)
            run_pass(m_box_shader, i == cascade_passes - 1);
        download(m_D);
    }

    void smooth_gaussian(std::vector<float>& m_D, float sigma)
    {
        upload(m_D);
        m_gaussian_shader.setUniform("sigma", sigma);
        run_pass(m_gaussian_shader, true);
        download(m_D);
    }

private:
    int m_W = 0, m_H = 0;
    int m_ping = 0; // which pingpong buffer is current source

    sf::RenderTexture m_tex;           // upload target
    sf::RenderTexture m_pingpong[2];   // swap chain for cascade passes
    sf::Shader m_box_shader;
    sf::Shader m_gaussian_shader;
    sf::Image m_readback;              // reused to avoid realloc

    sf::RenderStates states;

    void upload(const std::vector<float>& m_D)
    {
        // Pack float density into RGBA — store in R channel, normalised 0-255
        // Find max for normalisation
        float maxVal = *std::max_element(m_D.begin(), m_D.end());
        if (maxVal == 0.f) maxVal = 1.f;

        std::vector<std::uint8_t> pixels(m_W * m_H * 4);
        for (int i = 0; i < m_W * m_H; i++)
        {
            std::uint8_t v = (std::uint8_t)std::clamp(m_D[i] / maxVal * 255.f, 0.f, 255.f);
            pixels[i * 4 + 0] = v; // R
            pixels[i * 4 + 1] = v; // G
            pixels[i * 4 + 2] = v; // B
            pixels[i * 4 + 3] = 255;
        }

        sf::Image img({ (unsigned)m_W, (unsigned)m_H }, pixels.data());
 
        sf::Texture uploadTex;
        uploadTex.loadFromImage(img);

        // Draw into pingpong[0] as starting point
        m_ping = 0;
        sf::Sprite s(uploadTex);
        m_pingpong[0].clear();
        m_pingpong[0].draw(s);
        m_pingpong[0].display();

        m_maxVal = maxVal; // save for download
    }

    void run_pass(sf::Shader& shader, bool /*last*/)
    {
        int src = m_ping;
        int dst = 1 - m_ping;

        shader.setUniform("texture", sf::Shader::CurrentTexture);
        shader.setUniform("texelSize", sf::Glsl::Vec2(
            1.f / m_W, 1.f / m_H
        ));

        sf::Sprite s(m_pingpong[src].getTexture());
        states.shader = &shader;

        m_pingpong[dst].clear();
        m_pingpong[dst].draw(s, states);
        m_pingpong[dst].display();

        m_ping = dst;
    }

    void download(std::vector<float>& m_D)
    {
        // Read pixels back from current pingpong buffer
        sf::Image img = m_pingpong[m_ping].getTexture().copyToImage();
        const std::uint8_t* pixels = img.getPixelsPtr();

        for (int i = 0; i < m_W * m_H; i++)
            m_D[i] = (pixels[i * 4] / 255.f) * m_maxVal; // unpack R channel
    }

    float m_maxVal = 1.f;
};