// Fan Art

#include "fanart.hpp"
#include "pngu.h"
#include "boxmesh.hpp"
#include "text.hpp"
#include "gecko.h"

using namespace std;

static  guVector  _GRRaxisx = (guVector){1, 0, 0}; // DO NOT MODIFY!!!
static  guVector  _GRRaxisy = (guVector){0, 1, 0}; // Even at runtime
static  guVector  _GRRaxisz = (guVector){0, 0, 1}; // NOT ever!

CFanart::CFanart(void)
	: m_loaded(false), m_cfg(), m_bg(), m_bglq()
{
}

CFanart::~CFanart(void)
{
}

void CFanart::unload()
{
	m_cfg.unload();
	m_loaded = false;
	m_elms.clear();
}

bool CFanart::load(const char *path, const char *id)
{
	bool retval = false;
	unload();
	
	const char *dir = fmt("%s/%s", path, id);
	STexture fanBg, fanBgLq;
	
	STexture::TexErr texErr = fanBg.fromPNGFile(sfmt("%s/background.png", dir).c_str(), GX_TF_RGBA8, ALLOC_MEM2);
	if (texErr == STexture::TE_ERROR)
	{
		dir = fmt("%s/%.3s", path, id);
		texErr = fanBg.fromPNGFile(sfmt("%s/background.png", dir).c_str(), GX_TF_RGBA8, ALLOC_MEM2);
	}

	if (texErr == STexture::TE_OK)
	{
		m_cfg.load(sfmt("%s/%s.ini", dir, id).c_str());
		if (!m_cfg.loaded())
		{
			m_cfg.load(sfmt("%s/%.3s.ini", dir, id).c_str());
		}
		fanBgLq.fromPNGFile(sfmt("%s/background_lq.png", dir).c_str(), GX_TF_RGBA8, ALLOC_MEM2);
		
		for (int i = 1; i <= 6; i++)
		{
			CFanartElement elm(m_cfg, dir, i);
			gprintf("Loading fanart %d...", i);
			if (elm.IsValid())
			{
				m_elms.push_back(elm);
				gprintf("done\n");
			}
			else
				gprintf("failed\n");
		}
		
		m_loaded = true;
		retval = true;
	}
	
	m_bg = fanBg;
	m_bglq = fanBgLq;
	
	return retval;
}

void CFanart::getBackground(STexture &hq, STexture &lq)
{
	if (m_loaded)
	{
		hq = m_bg;
		lq = m_bglq;
	}
}

u32 CFanart::getTextColor(void)
{
	return m_cfg.getInt("general", "textcolor", 0xFFFFFFFF);
}

bool CFanart::hideCover(void)
{
	bool retval = m_cfg.getBool("general", "hidecover", false);
	if (!retval && m_cfg.getBool("general", "show_cover_after_animation", false)) // Show the cover after the animation is finished
	{
		retval = true;
		for (u32 i = 0; i < m_elms.size(); ++i)
		{
			if (!m_elms[i].IsAnimationComplete())
			{
				retval = false;
				break;
			}
		}
	}
	return retval;
}

void CFanart::tick()
{
	for (u32 i = 0; i < m_elms.size(); ++i)
		m_elms[i].tick();
}

void CFanart::draw(bool front)
{
	GX_SetNumChans(1);
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GX_SetNumTexGens(1);
	GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
	GX_SetAlphaUpdate(GX_TRUE);
	GX_SetCullMode(GX_CULL_NONE);
	GX_SetZMode(GX_DISABLE, GX_LEQUAL, GX_TRUE);
	
	for (u32 i = 0; i < m_elms.size(); ++i)
		if ((front && m_elms[i].ShowOnTop()) || !front)
			m_elms[i].draw();
}

CFanartElement::CFanartElement(Config &cfg, const char *dir, int artwork)
	: m_artwork(artwork), m_isValid(false)
{
	m_isValid = m_art.fromPNGFile(sfmt("%s/artwork%d.png", dir, artwork).c_str(), GX_TF_RGBA8, ALLOC_MEM2) == STexture::TE_OK;
	if (!m_isValid)
		return;

	const char *section = fmt("artwork%d", artwork);
	
	m_show_on_top = cfg.getBool(section, "show_on_top", true);
	
	m_x = cfg.getInt(section, "x", 0);
	m_y = cfg.getInt(section, "y", 0);
	m_scaleX = cfg.getFloat(section, "scale_x", 1.f);
	m_scaleY = cfg.getFloat(section, "scale_y", 1.f);
	m_alpha = min(cfg.getInt(section, "alpha", 255), 255);
	m_delay = (int) (cfg.getFloat(section, "delay", 0.f) * 50);
	m_angle = cfg.getFloat(section, "angle", 0.f);

	m_event_duration = (int) (cfg.getFloat(section, "duration", 0.f) * 50);
	m_event_x = m_event_duration == 0 ? m_x : cfg.getInt(section, "event_x", m_x);
	m_event_y = m_event_duration == 0 ? m_y : cfg.getInt(section, "event_y", m_y);
	m_event_scaleX = m_event_duration == 0 ? m_scaleX : cfg.getInt(section, "event_scale_x", m_scaleX);
	m_event_scaleY = m_event_duration == 0 ? m_scaleY : cfg.getInt(section, "event_scale_y", m_scaleY);
	m_event_alpha = m_event_duration == 0 ? m_alpha : min(cfg.getInt(section, "event_alpha", m_alpha), 255); // Not from m_alpha, because the animation can start less translucent than m_alpha
	m_event_angle = m_event_duration == 0 ? m_angle : cfg.getFloat(section, "event_angle", m_angle);
	
	m_step_x = m_event_duration == 0 ? 0 : (m_x - m_event_x) / m_event_duration;
	m_step_y = m_event_duration == 0 ? 0 : (m_y - m_event_y) / m_event_duration;
	m_step_scaleX = m_event_duration == 0 ? 0 : (m_scaleX - m_event_scaleX) / m_event_duration;
	m_step_scaleY = m_event_duration == 0 ? 0 : (m_scaleY - m_event_scaleY) / m_event_duration;
	m_step_alpha = m_event_duration == 0 ? 0 : (m_alpha - m_event_alpha) / m_event_duration;
	m_step_angle = m_event_duration == 0 ? 0 : (m_angle - m_event_angle) / m_event_duration;
	
	gprintf("Loading element %d x: %d, y: %d, scale x: %f, scale y: %f, alpha: %d\n", m_artwork, m_x, m_y, m_scaleX, m_scaleY, m_alpha);
	gprintf("             Event x: %d, y: %d, scale x: %f, scale y: %f, alpha: %d\n", m_event_x, m_event_y, m_event_scaleX, m_event_scaleY, m_event_alpha);
}

CFanartElement::~CFanartElement(void)
{
}

bool CFanartElement::IsValid()
{
	return m_isValid;
}

bool CFanartElement::IsAnimationComplete()
{
	return m_event_duration == 0;
}

bool CFanartElement::ShowOnTop()
{
	return m_show_on_top;
}

void CFanartElement::tick()
{
	if (m_delay > 0)
	{
		m_delay--;
		return;
	}

	if ((m_step_x < 0 && m_event_x > m_x) || (m_step_x > 0 && m_event_x < m_x))
		m_event_x = (int) (m_event_x + m_step_x);
	if ((m_step_y < 0 && m_event_y > m_y) || (m_step_y > 0 && m_event_y < m_y))
		m_event_y = (int) (m_event_y + m_step_y);
	if ((m_step_alpha < 0 && m_event_alpha > m_alpha) || (m_step_alpha > 0 && m_event_alpha < m_alpha))
		m_event_alpha = (int) (m_event_alpha + m_step_alpha);
	if ((m_step_scaleX < 0 && m_event_scaleX > m_scaleX) || (m_step_scaleX > 0 && m_event_scaleX < m_scaleX))
		m_event_scaleX += m_step_scaleX;
	if ((m_step_scaleY < 0 && m_event_scaleY > m_scaleY) || (m_step_scaleY > 0 && m_event_scaleY < m_scaleY))
		m_event_scaleY += m_step_scaleY;
	if ((m_step_angle < 0 && m_event_angle > m_angle) || (m_step_angle > 0 && m_event_angle < m_angle))
		m_event_angle = (int) (m_event_angle + m_step_angle);
	
	if (m_event_duration > 0)
	{
		m_event_duration--;
	}
}

void CFanartElement::draw()
{
	if (m_delay > 0) return;
	
	GXTexObj artwork;
	Mtx modelViewMtx, idViewMtx, rotViewMtxZ;

	float w;
	float h;
	u8 alpha = m_alpha;
	float scaleX = 1.f;
	float scaleY = 1.f;
	
	if (alpha == 0 || scaleX == 0.f || scaleY == 0.f) // Nothing to draw here
		return;

	guMtxIdentity(idViewMtx);
	guMtxScaleApply(idViewMtx, idViewMtx, m_event_scaleX, m_event_scaleY, 1.f);
	
	guMtxRotAxisDeg(rotViewMtxZ, &_GRRaxisz, m_event_angle);
	guMtxConcat(rotViewMtxZ, idViewMtx, modelViewMtx);
	
	guMtxTransApply(modelViewMtx, modelViewMtx, m_event_x, m_event_y, 0.f);
	GX_LoadPosMtxImm(modelViewMtx, GX_PNMTX0);

	GX_InitTexObj(&artwork, m_art.data.get(), m_art.width, m_art.height, m_art.format, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_LoadTexObj(&artwork, GX_TEXMAP0);
	
	w = (float)(m_art.width / 2); // * m_event_scaleX;
	h = (float)(m_art.height / 2); // * m_event_scaleY;

	GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
	
	// Draw top left
	GX_Position3f32(-w, -h, 0.f);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(0.f, 0.f);
	
	// Draw top right
	GX_Position3f32(w, -h, 0.f);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(1.f, 0.f);
	
	// Draw bottom right
	GX_Position3f32(w, h, 0.f);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(1.f, 1.f);
	
	// Draw bottom left
	GX_Position3f32(-w, h, 0.f);
	GX_Color4u8(0xFF, 0xFF, 0xFF, alpha);
	GX_TexCoord2f32(0.f, 1.f);
	GX_End();
}