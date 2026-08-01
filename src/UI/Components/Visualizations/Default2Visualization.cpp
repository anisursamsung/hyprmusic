#include "Default2Visualization.hpp"
#include <algorithm>
#include <random>

namespace UI::Components {

CSharedPointer<IElement> Default2Visualization::build(CSharedPointer<CPalette> palette) {
  m_container = CRectangleBuilder::begin()
                    ->color([] { return CHyprColor(0, 0, 0, 0); })
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_PERCENT,
                                        CDynamicSize::HT_SIZE_PERCENT,
                                        {1.0F, 1.0F}))
                    ->commence();

  m_balls.clear();
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> posDist(0.1f, 0.9f);
  std::uniform_real_distribution<float> velDist(-0.005f, 0.005f);
  std::uniform_real_distribution<float> sizeDist(8.0f, 24.0f);
  std::uniform_real_distribution<float> colorDist(0.3f, 1.0f); // Bright, vibrant range

  for (int i = 0; i < 32; ++i) {
    KineticBall ball;
    ball.x = posDist(gen);
    ball.y = posDist(gen);
    ball.dx = velDist(gen);
    ball.dy = velDist(gen);
    ball.baseSize = sizeDist(gen);

    // Generate a random bright color with 80% opacity for a nice glow overlay effect
    ball.color = CHyprColor(colorDist(gen), colorDist(gen), colorDist(gen), 0.8f);

    ball.elem = CRectangleBuilder::begin()
                    ->color([c = ball.color] {
                      return c; // Use the ball's unique color
                    })
                    ->rounding(100) 
                    ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                                        CDynamicSize::HT_SIZE_ABSOLUTE,
                                        {ball.baseSize, ball.baseSize}))
                    ->commence();

    ball.elem->setPositionMode(IElement::HT_POSITION_ABSOLUTE);
    ball.elem->setPositionFlag(IElement::HT_POSITION_FLAG_LEFT, true);
    ball.elem->setPositionFlag(IElement::HT_POSITION_FLAG_TOP, true);

    m_balls.push_back(ball);
    m_container->addChild(ball.elem);
  }

  return m_container;
}

void Default2Visualization::update(const std::vector<float>& spectrum) {
  auto parentSize = m_container->size();
  if (parentSize.x <= 0 || parentSize.y <= 0) return; 

  for (size_t i = 0; i < m_balls.size(); ++i) {
    auto& ball = m_balls[i];
    
    // Physics: Kinetic acceleration based on frequency amplitude
    float energy = spectrum[i]; 
    float speedMult = 1.0f + (energy * 8.0f); 

    ball.x += ball.dx * speedMult;
    ball.y += ball.dy * speedMult;

    // Boundary Bouncing
    if (ball.x <= 0.0f || ball.x >= 1.0f) { 
        ball.dx *= -1.0f; 
        ball.x = std::clamp(ball.x, 0.0f, 1.0f); 
    }
    if (ball.y <= 0.0f || ball.y >= 1.0f) { 
        ball.dy *= -1.0f; 
        ball.y = std::clamp(ball.y, 0.0f, 1.0f); 
    }

    // Reactivity: Grow massively on beat drops
    float currentSize = ball.baseSize + (energy * 120.0f); 
    
    ball.elem->rebuild()
        ->size(CDynamicSize(CDynamicSize::HT_SIZE_ABSOLUTE,
                            CDynamicSize::HT_SIZE_ABSOLUTE,
                            {currentSize, currentSize}))
        ->rounding(static_cast<int>(currentSize / 2.0f))
        ->commence();

    float maxX = std::max(0.0f, static_cast<float>(parentSize.x) - currentSize);
    float maxY = std::max(0.0f, static_cast<float>(parentSize.y) - currentSize);

    float targetX = ball.x * maxX;
    float targetY = ball.y * maxY;

    ball.elem->setAbsolutePosition(Hyprutils::Math::Vector2D(targetX, targetY));
  }
}

} // namespace UI::Components
