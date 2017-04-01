/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "boss.h"
#include "global.h"
#include "stage.h"
#include <string.h>
#include <stdio.h>

Boss* create_boss(char *name, char *ani, char *dialog, complex pos) {
	Boss *buf = malloc(sizeof(Boss));
	memset(buf, 0, sizeof(Boss));

	buf->name = malloc(strlen(name) + 1);
	strcpy(buf->name, name);
	buf->pos = pos;

	buf->ani = get_ani(ani);
	if(dialog)
		buf->dialog = get_tex(dialog);

	return buf;
}

void draw_boss_text(Alignment align, float x, float y, const char *text) {
	glColor4f(0,0,0,1);
	draw_text(align, x+1, y+1, text, _fonts.standard);
	glColor4f(1,1,1,1);
	draw_text(align, x, y, text, _fonts.standard);
}

void spell_opening(Boss *b, int time) {
	complex x0 = VIEWPORT_W/2+I*VIEWPORT_H/2;
	float f = clamp((time-40.)/60.,0,1);

	complex x = x0 + (VIEWPORT_W+I*35 - x0) * f*(f+1)*0.5;

	int strw = stringwidth(b->current->name,_fonts.standard);

	glPushMatrix();
	glTranslatef(creal(x),cimag(x),0);
	float scale = f+1.*(1-f)*(1-f)*(1-f);
	glScalef(scale,scale,1);
	glRotatef(360*f,1,1,0);
	glDisable(GL_CULL_FACE);
	draw_boss_text(AL_Right, strw/2*(1-f), 0, b->current->name);
	glEnable(GL_CULL_FACE);
	glPopMatrix();


	glUseProgram(0);

}

void draw_extraspell_bg(Boss *boss, int time) {
	// overlay for all extra spells

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glColor4f(0.2,0.1,0,0.7);
	fill_screen(sin(time) * 0.015, time / 50.0, 1, "stage3/wspellclouds");
	glColor4f(1,1,1,1);
	glBlendEquation(GL_MIN);
	fill_screen(cos(time) * 0.015, time / 70.0, 1, "stage4/kurumibg2");
	fill_screen(sin(time+2.1) * 0.015, time / 30.0, 1, "stage4/kurumibg2");
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
}

void draw_boss(Boss *boss) {
	draw_animation_p(creal(boss->pos), cimag(boss->pos) + 6*sin(global.frames/25.0), boss->anirow, boss->ani);
	draw_boss_text(AL_Left, 10, 20, boss->name);

	if(!boss->current)
		return;

	if(boss->current->type == AT_Spellcard || boss->current->type == AT_SurvivalSpell || boss->current->type == AT_ExtraSpell)
		spell_opening(boss, global.frames - boss->current->starttime);

	if(boss->current->type != AT_Move) {
		char buf[16];
		snprintf(buf, sizeof(buf),  "%.2f", max(0, (boss->current->timeout - global.frames + boss->current->starttime)/(float)FPS));
		draw_boss_text(AL_Center, VIEWPORT_W - 20, 10, buf);

		int nextspell, lastspell;
		for(nextspell = 0; nextspell < boss->acount - 1; nextspell++) {
			int t = boss->attacks[nextspell].type;
			if(boss->dmg < boss->attacks[nextspell].dmglimit && t == AT_Spellcard)
				break;
		}

		for(lastspell = nextspell; lastspell > 0; lastspell--) {
			int t = boss->attacks[lastspell].type;
			if(boss->dmg > boss->attacks[lastspell].dmglimit && t == AT_Spellcard)
				break;
		}

		int dmgoffset = boss->attacks[lastspell].dmglimit;
		int dmgspan = boss->attacks[nextspell].dmglimit - boss->attacks[lastspell].dmglimit;

		glPushMatrix();
		glTranslatef(VIEWPORT_W-50, 4, 0);
		glScalef((VIEWPORT_W-60)/(float)dmgspan,1,1);
		glTranslatef(dmgoffset,0,0);
		int i;

		glColor4f(0,0,0,0.65);

		glPushMatrix();
		glTranslatef(-(boss->attacks[nextspell].dmglimit+boss->dmg-2)*0.5, 2, 0);
		glScalef(boss->attacks[nextspell].dmglimit-boss->dmg+2, 4, 1);

		draw_quad();
		glPopMatrix();

		for(i = nextspell; i >= 0; i--) {
			if(boss->dmg > boss->attacks[i].dmglimit)
				continue;

			switch(boss->attacks[i].type) {
			case AT_Normal:
				glColor3f(1,1,1);
				break;
			case AT_Spellcard:
				glColor3f(1,0.65,0.65);
				break;
			case AT_SurvivalSpell:
				glColor3f(0.5,0.5,1);
			case AT_ExtraSpell:
				glColor3f(1.0, 0.3, 0.2);
				break;
			default:
				break; // never happens
			}

			glPushMatrix();
			glTranslatef(-(boss->attacks[i].dmglimit+boss->dmg)*0.5, 1, 0);
			glScalef(boss->attacks[i].dmglimit-boss->dmg, 2, 1);

			draw_quad();
			glPopMatrix();
		}

		glPopMatrix();

		glColor4f(1,1,1,0.7);

		int x = 0;
		for(i = boss->acount-1; i > nextspell; i--)
			if(boss->attacks[i].type == AT_Spellcard)
				draw_texture(x += 22, 40, "star");

		glColor3f(1,1,1);
	}
}

void boss_rule_extra(Boss *boss, float alpha) {
	int cnt = 10 * max(1, alpha);
	alpha = min(2, alpha);
	int lt = 1;

	if(alpha == 0) {
		lt += 2;
		alpha = 1 * frand();
	}

	int i; for(i = 0; i < cnt; ++i) {
		float a = i*2*M_PI/cnt + global.frames / 100.0;

		complex dir = cexp(I*(a+global.frames/50.0));
		complex pos = boss->pos + dir * (100 + 50 * psin(alpha*global.frames/10.0+2*i)) * alpha;
		complex vel = dir * 3;

		float r, g, b;
		float v = max(0, alpha - 1);
		float psina = psin(a);

		r = 1.0 - 0.5 * psina *    v;
		g = 0.5 + 0.2 * psina * (1-v);
		b = 0.5 + 0.5 * psina *    v;

		create_particle2c("flare", pos, rgb(r, g, b), FadeAdd, timeout_linear, 15*lt, vel);

		int d = 5;
		if(!(global.frames % d))
			create_particle3c((frand() < v*0.5 || lt > 1)? "stain" : "boss_shadow", pos, rgb(r, g, b), GrowFadeAdd, timeout_linear, 30*lt, vel * (1 - 2 * !(global.frames % (2*d))), 2.5);
	}
}

void boss_kill_projectiles(void) {
	Projectile *p;
	for(p = global.projs; p; p = p->next)
		if(p->type == FairyProj)
			p->type = DeadProj;
	delete_lasers();
}

bool boss_is_dying(Boss *boss) {
	return boss->current && boss->current->endtime && boss->current->type != AT_Move && boss->current - boss->attacks >= boss->acount-1;
}

void process_boss(Boss **pboss) {
	Boss *boss = *pboss;

	if(!boss->current)
		return;

	int time = global.frames - boss->current->starttime;
	bool extra = boss->current->type == AT_ExtraSpell;
	bool over = boss->current->finished && global.frames >= boss->current->endtime;

	// TODO: mark uncaptured normal spells as failed too (for spell bonuses and player stats)

	if(!boss->current->endtime)
		boss->current->rule(boss, time);

	if(extra) {
		float base = 0.2;
		float ampl = 0.2;
		float s = sin(time / 90.0 + M_PI*1.2);

		if(boss->current->endtime) {
			float p = (boss->current->endtime - global.frames)/(float)ATTACK_END_DELAY_EXTRA;
			float a = max((base + ampl * s) * p * 0.5, 5 * pow(1 - p, 3));
			if(a < 2) {
				global.shake_view = 3 * a;
				boss_rule_extra(boss, a);
				if(a > 1) {
					boss_rule_extra(boss, a * 0.5);
					if(a > 1.3) {
						global.shake_view = 5 * a;
						if(a > 1.7)
							global.shake_view += 2 * a;
						boss_rule_extra(boss, 0);
						boss_rule_extra(boss, 0.1);
					}
				}
			} else {
				over = 1;
				global.shake_view_fade = 0.15;
			}
		} else if(time < 0) {
			boss_rule_extra(boss, 1+time/(float)ATTACK_START_DELAY_EXTRA);
		} else {
			float o = min(0, -5 + time/30.0);
			float q = (time <= 150? 1 - pow(time/250.0, 2) : min(1, time/60.0));

			boss_rule_extra(boss, max(1-time/300.0, base + ampl * s) * q);
			if(o) {
				boss_rule_extra(boss, max(1-time/300.0, base + ampl * s) - o);
				if(!global.shake_view) {
					global.shake_view = 5;
					global.shake_view_fade = 0.9;
				} else if(o > -0.05) {
					global.shake_view = 10;
					global.shake_view_fade = 0.5;
				}
			}
		}
	}

	if(!boss->current->endtime && (boss->current->type != AT_Move && boss->dmg >= boss->current->dmglimit || time > boss->current->timeout)) {
		int delay = extra ? ATTACK_END_DELAY_EXTRA : ATTACK_END_DELAY;
		if(boss->current-boss->attacks >= boss->acount-1)
			delay = BOSS_DEATH_DELAY;
		if(boss->current->type == AT_Move) // do not delay if the boss is running away at the end
			delay = 0;
		boss->current->endtime = global.frames + delay;
		boss->current->finished = FINISH_WIN;
		boss->current->rule(boss, EVENT_DEATH);
		boss_kill_projectiles();
	}

	if(boss_is_dying(boss)) {
		float t = (global.frames-boss->current->endtime)/(float)BOSS_DEATH_DELAY+1;
		complex pos = boss->pos;
		Color c = rgba(0.1+sin(10*t),0.1+cos(10*t),0.5,t);
		tsrand_fill(6);
		create_particle4c("petal", pos, c, Petal, asymptotic, sign(anfrand(5))*(3+t*5*afrand(0))*cexp(I*M_PI*8*t), 5+I, afrand(2) + afrand(3)*I, afrand(4) + 360.0*I*afrand(1));

	}

	if(over) {
		if(extra && boss->current->finished == FINISH_WIN)
			spawn_items(boss->pos, Life, 1, NULL);

		boss->dmg = boss->current->dmglimit + 1;
		boss->current++;
		if(boss->current - boss->attacks < boss->acount)
			start_attack(boss, boss->current);
		else {
			boss->current = NULL;
			boss_death(pboss);
		}
	}

}

void boss_death(Boss **boss) {
	if((*boss)->acount && (*boss)->attacks[(*boss)->acount-1].type != AT_Move)
		petal_explosion(35, (*boss)->pos);

	free_boss(*boss);
	*boss = NULL;
	boss_kill_projectiles();
}

void free_boss(Boss *boss) {
	del_ref(boss);

	for(int i = 0; i < boss->acount; i++)
		free_attack(&boss->attacks[i]);

	free(boss->name);
	free(boss->attacks);
}

void free_attack(Attack *a) {
	free(a->name);
}

void start_attack(Boss *b, Attack *a) {
	log_debug("%s", a->name);

	if(global.replaymode == REPLAY_RECORD && global.stage->type == STAGE_STORY && !global.continues) {
		StageInfo *i = stage_get_by_spellcard(a->info, global.diff);
		if(i) {
			StageProgress *p = stage_get_progress_from_info(i, global.diff, true);
			if(p && !p->unlocked) {
				log_info("Spellcard unlocked! %s: %s", i->title, i->subtitle);
				p->unlocked = true;
			}
		}
#if DEBUG
		else if(a->type == AT_Spellcard || a->type == AT_ExtraSpell) {
			log_warn("FIXME: spellcard '%s' is not available in spell practice mode!", a->name);
		}
#endif
	}

	a->starttime = global.frames + (a->type == AT_ExtraSpell? ATTACK_START_DELAY_EXTRA : ATTACK_START_DELAY);
	a->rule(b, EVENT_BIRTH);
	if(a->type == AT_Spellcard || a->type == AT_SurvivalSpell || a->type == AT_ExtraSpell) {
		play_sound("charge_generic");
		for(int i = 0; i < 10+10*(a->type == AT_ExtraSpell); i++) {
			tsrand_fill(4);
			create_particle2c("stain", VIEWPORT_W/2 + VIEWPORT_W/4*anfrand(0)+I*VIEWPORT_H/2+I*anfrand(1)*30, rgb(0.2,0.3,0.4), GrowFadeAdd, timeout_linear, 50, sign(anfrand(2))*10*(1+afrand(3)));
		}
	}

	Projectile *p;
	for(p = global.projs; p; p = p->next)
		if(p->type == FairyProj)
			p->type = DeadProj;

	delete_lasers();
}

Attack* boss_add_attack(Boss *boss, AttackType type, char *name, float timeout, int hp, BossRule rule, BossRule draw_rule) {
	boss->attacks = realloc(boss->attacks, sizeof(Attack)*(++boss->acount));
	Attack *a = &boss->attacks[boss->acount-1];
	memset(a, 0, sizeof(Attack));

	boss->current = &boss->attacks[0];

	a->type = type;
	a->name = strdup(name);
	a->timeout = timeout * FPS;

	int dmg = 0;
	if(boss->acount > 1)
		dmg = boss->attacks[boss->acount - 2].dmglimit;

	a->dmglimit = dmg + hp;
	a->rule = rule;
	a->draw_rule = draw_rule;

	a->starttime = global.frames;
	a->endtime = 0;
	a->finished = FINISH_NOPE;
	return a;
}

void boss_generic_move(Boss *b, int time) {
	if(b->current->info->pos_dest != BOSS_NOMOVE) {
		GO_TO(b, b->current->info->pos_dest, 0.1)
	}
}

Attack* boss_add_attack_from_info(Boss *boss, AttackInfo *info, char move) {
	if(move) {
		boss_add_attack(boss, AT_Move, "Generic Move", 1, 0, boss_generic_move, NULL)->info = info;
	}

	Attack *a = boss_add_attack(boss, info->type, info->name, info->timeout, info->hp, info->rule, info->draw_rule);
	a->info = info;
	return a;
}

void boss_preload(void) {
	preload_resources(RES_SFX, RESF_OPTIONAL,
		"charge_generic",
	NULL);

	preload_resources(RES_TEXTURE, RESF_DEFAULT,
		"boss_spellcircle0",
		"boss_circle",
		"boss_indicator",
	NULL);

	preload_resources(RES_SHADER, RESF_DEFAULT,
		"boss_zoom",
		"spellcard_intro",
		"spellcard_outro",
	NULL);

	StageInfo *s = global.stage;

	if(s->type != STAGE_SPELL || s->spell->type == AT_ExtraSpell) {
		preload_resources(RES_TEXTURE, RESF_DEFAULT,
			"stage3/wspellclouds",
			"stage4/kurumibg2",
		NULL);
	}
}
