#include "edje_private.h"

static void _edje_part_make_rtl(Edje_Part_Description_Common *desc);
static Edje_Part_Description_Common *_edje_get_description_by_orientation(Edje *ed, Edje_Part_Description_Common *src, Edje_Part_Description_Common **dst, unsigned char type);

static void _edje_part_recalc_single(Edje *ed, Edje_Real_Part *ep,
                                     Edje_Part_Description_Common *desc, Edje_Part_Description_Common *chosen_desc,
                                     Edje_Real_Part *center, Edje_Real_Part *light, Edje_Real_Part *persp,
                                     Edje_Real_Part *rel1_to_x, Edje_Real_Part *rel1_to_y,
                                     Edje_Real_Part *rel2_to_x, Edje_Real_Part *rel2_to_y,
                                     Edje_Real_Part *confine_to, Edje_Calc_Params *params,
                                     FLOAT_T pos);

void
_edje_part_pos_set(Edje *ed, Edje_Real_Part *ep, int mode, FLOAT_T pos, FLOAT_T v1, FLOAT_T v2)
{
   FLOAT_T fp_pos;
   FLOAT_T npos;

   pos = CLAMP(pos, ZERO, FROM_INT(1));

   fp_pos = pos;

#if 0 // old code - easy to enable for comparing float vs fixed point
   /* take linear pos along timescale and use interpolation method */
   switch (mode)
     {
      case EDJE_TWEEN_MODE_SINUSOIDAL:
	 /* npos = (1.0 - cos(pos * PI)) / 2.0; */
	 npos = DIV2(SUB(FROM_INT(1),
			 COS(MUL(fp_pos,
				 PI))));
	 break;
      case EDJE_TWEEN_MODE_ACCELERATE:
	 /* npos = 1.0 - sin((PI / 2.0) + (pos * PI / 2.0)); */
	 npos = SUB(FROM_INT(1),
		    SIN(ADD(DIV2(PI),
			    MUL(fp_pos,
				DIV2(PI)))));
	 break;
      case EDJE_TWEEN_MODE_DECELERATE:
	 /* npos = sin(pos * PI / 2.0); */
	 npos = SIN(MUL(fp_pos,
			DIV2(PI)));
	break;
      case EDJE_TWEEN_MODE_LINEAR:
	 npos = fp_pos;
	 break;
      default:
         npos = fp_pos;
         break;
     }
#else
   switch (mode & EDJE_TWEEN_MODE_MASK)
     {
      case EDJE_TWEEN_MODE_SINUSOIDAL:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_SINUSOIDAL,
                                                  0.0, 0.0));
        break;
      case EDJE_TWEEN_MODE_ACCELERATE:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_ACCELERATE,
                                                  0.0, 0.0));
        break;
      case EDJE_TWEEN_MODE_DECELERATE:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_DECELERATE,
                                                  0.0, 0.0));
        break;
      case EDJE_TWEEN_MODE_LINEAR:
        npos = fp_pos;
/*        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_LINEAR, 
                                                  0.0, 0.0));
 */
        break;
      case EDJE_TWEEN_MODE_ACCELERATE_FACTOR:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_ACCELERATE_FACTOR,
                                                  TO_DOUBLE(v1), 0.0));
        break;
      case EDJE_TWEEN_MODE_DECELERATE_FACTOR:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_DECELERATE_FACTOR,
                                                  TO_DOUBLE(v1), 0.0));
        break;
      case EDJE_TWEEN_MODE_SINUSOIDAL_FACTOR:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_SINUSOIDAL_FACTOR,
                                                  TO_DOUBLE(v1), 0.0));
        break;
      case EDJE_TWEEN_MODE_DIVISOR_INTERP:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_DIVISOR_INTERP,
                                                  TO_DOUBLE(v1), TO_DOUBLE(v2)));
        break;
      case EDJE_TWEEN_MODE_BOUNCE:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_BOUNCE,
                                                  TO_DOUBLE(v1), TO_DOUBLE(v2)));
        break;
      case EDJE_TWEEN_MODE_SPRING:
        npos = FROM_DOUBLE(ecore_animator_pos_map(TO_DOUBLE(pos),
                                                  ECORE_POS_MAP_SPRING,
                                                  TO_DOUBLE(v1), TO_DOUBLE(v2)));
        break;
      default:
        npos = fp_pos;
        break;
     }
#endif
   if (npos == ep->description_pos) return;

   ep->description_pos = npos;

   ed->dirty = EINA_TRUE;
   ed->recalc_call = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   ep->invalidate = 1;
#endif
}


/**
 * Returns part description
 *
 * @internal
 *
 * Converts part description to RTL-desc.
 *
 * @param desc Pointer to desc buffer.
 *
 **/
static void
_edje_part_make_rtl(Edje_Part_Description_Common *desc)
{
   double t;
   int i;

   if(!desc)
     return;

   /* This makes alignment right-oriented */
   desc->align.x = 1.0 - desc->align.x;

   /* same as above for relative components */
   t = desc->rel1.relative_x;
   desc->rel1.relative_x = 1.0 - desc->rel2.relative_x;
   desc->rel2.relative_x = 1.0 - t;

   /* +1 and +1 are because how edje works with right
    * side borders - nothing is printed beyond that limit
    *
    * rel2 is now to the left of rel1, and Edje assumes
    * the opposite so we switch corners on x-axis to define
    * offset from right to left */
   i = desc->rel1.offset_x;
   desc->rel1.offset_x = -(desc->rel2.offset_x + 1);
   desc->rel2.offset_x = -(i + 1);

   i = desc->rel1.id_x;
   desc->rel1.id_x = desc->rel2.id_x;
   desc->rel2.id_x = i;
}

/**
 * Returns part description
 *
 * @internal
 *
 * Returns part description according to object orientation.
 * When object is in RTL-orientation (RTL flag is set)
 * this returns the RTL-desc of it.
 * RTL-desc would be allocated if was not created by a previous call.
 * The dst pointer is updated in case of an allocation.
 *
 * @param ed Edje object.
 * @param src The Left To Right (LTR), original desc.
 * @param dst Pointer to Right To Left (RTL) desc-list.
 * @param type name of dec type. Example: "default".
 *
 * @return Edje part description.
 *
 **/
static Edje_Part_Description_Common *
_edje_get_description_by_orientation(Edje *ed, Edje_Part_Description_Common *src, Edje_Part_Description_Common **dst, unsigned char type)
{
   Edje_Part_Description_Common *desc_rtl = NULL;
   Edje_Part_Collection_Directory_Entry *ce;
   size_t memsize = 0;

   /* RTL flag is not set, return original description */
   if(!edje_object_mirrored_get(ed->obj))
      return src;

   if(*dst)
     return *dst; /* Was allocated before and we should use it */

#define EDIT_ALLOC_POOL_RTL(Short, Type, Name)                          \
         case EDJE_PART_TYPE_##Short:                                   \
           {                                                            \
              Edje_Part_Description_##Type *Name;                       \
              Name = eina_mempool_malloc(ce->mp_rtl.Short,              \
                    sizeof (Edje_Part_Description_##Type));             \
              memset(Name, 0, sizeof(Edje_Part_Description_##Type));    \
              desc_rtl = &Name->common;                                 \
              memsize = sizeof(Edje_Part_Description_##Type);           \
              break;                                                    \
           }

   ce = eina_hash_find(ed->file->collection, ed->group);

   switch (type)
     {
      case EDJE_PART_TYPE_RECTANGLE:
         desc_rtl = eina_mempool_malloc(ce->mp_rtl.RECTANGLE,
                                        sizeof (Edje_Part_Description_Common));
         ce->count.RECTANGLE++;
         memsize = sizeof(Edje_Part_Description_Common);
         break;
      case EDJE_PART_TYPE_SWALLOW:
         desc_rtl = eina_mempool_malloc(ce->mp_rtl.SWALLOW,
                                        sizeof (Edje_Part_Description_Common));
         ce->count.SWALLOW++;
         memsize = sizeof(Edje_Part_Description_Common);
         break;
      case EDJE_PART_TYPE_GROUP:
         desc_rtl = eina_mempool_malloc(ce->mp_rtl.GROUP,
                                        sizeof (Edje_Part_Description_Common));
         ce->count.GROUP++;
         memsize = sizeof(Edje_Part_Description_Common);
         break;
     case EDJE_PART_TYPE_SPACER:
         desc_rtl = eina_mempool_malloc(ce->mp_rtl.SPACER,
                                        sizeof (Edje_Part_Description_Common));
         ce->count.SPACER++;
         memsize = sizeof(Edje_Part_Description_Common);
         break;
         EDIT_ALLOC_POOL_RTL(TEXT, Text, text);
         EDIT_ALLOC_POOL_RTL(TEXTBLOCK, Text, text);
         EDIT_ALLOC_POOL_RTL(IMAGE, Image, image);
         EDIT_ALLOC_POOL_RTL(PROXY, Proxy, proxy);
         EDIT_ALLOC_POOL_RTL(BOX, Box, box);
         EDIT_ALLOC_POOL_RTL(TABLE, Table, table);
         EDIT_ALLOC_POOL_RTL(EXTERNAL, External, external_params);
     }

   if (desc_rtl)
      memcpy(desc_rtl, src, memsize);

   _edje_part_make_rtl(desc_rtl);

   *dst = desc_rtl;
   return desc_rtl;
}

Edje_Part_Description_Common *
_edje_part_description_find(Edje *ed, Edje_Real_Part *rp, const char *name,
                            double val)
{
   Edje_Part *ep = rp->part;
   Edje_Part_Description_Common *ret = NULL;
   Edje_Part_Description_Common *d;

   double min_dst = 99999.0;
   unsigned int i;

   /* RTL flag is set, return RTL description */
   if(edje_object_mirrored_get(ed->obj))
     if(!ep->other.desc_rtl)
       ep->other.desc_rtl = (Edje_Part_Description_Common **)
          calloc(ep->other.desc_count,
                sizeof (Edje_Part_Description_Common *));

   if (!strcmp(name, "default") && val == 0.0)
     return _edje_get_description_by_orientation(ed,
           ep->default_desc, &ep->default_desc_rtl, ep->type);

   if (!strcmp(name, "custom"))
     return rp->custom ?
        _edje_get_description_by_orientation(ed, rp->custom->description,
              &rp->custom->description_rtl, ep->type) : NULL;

   if (!strcmp(name, "default"))
     {
        ret = _edje_get_description_by_orientation(ed, ep->default_desc,
                                                   &ep->default_desc_rtl,
                                                   ep->type);

        min_dst = ABS(ep->default_desc->state.value - val);
     }

   for (i = 0; i < ep->other.desc_count; ++i)
     {
        d = ep->other.desc[i];

        if (d->state.name && (d->state.name == name ||
                              !strcmp(d->state.name, name)))
          {
             double dst;

             dst = ABS(d->state.value - val);
             if (dst < min_dst)
               {
                  ret = _edje_get_description_by_orientation(ed, d,
                                                             &ep->other.desc_rtl[i], ep->type);
                  min_dst = dst;
               }
          }
     }

   return ret;
}

static int
_edje_image_find(Evas_Object *obj, Edje *ed, Edje_Real_Part_Set **eps, 
                 Edje_Part_Description_Image *st, Edje_Part_Image_Id *imid)
{
   Edje_Image_Directory_Set_Entry *entry;
   Edje_Image_Directory_Set *set = NULL;
   Eina_List *l;
   int w = 0, h = 0, id;
   
   if (!st && !imid)         return -1;
   if (st && !st->image.set) return st->image.id;
   if (imid && !imid->set)   return imid->id;
   
   if (imid) id = imid->id;
   else      id = st->image.id;
   
   evas_object_geometry_get(obj, NULL, NULL, &w, &h);
   if (eps && *eps)
     {
        if ((*eps)->id == id) set = (*eps)->set;
        if (set)
          {
             if (((*eps)->entry->size.min.w <= w) && 
                 (w <= (*eps)->entry->size.max.w))
               {
                  if (((*eps)->entry->size.min.h <= h) && 
                      (h <= (*eps)->entry->size.max.h))
                    {
                       return (*eps)->entry->id;
                    }
               }
          }
     }
   
   if (!set) set = ed->file->image_dir->sets + id;
   
   EINA_LIST_FOREACH(set->entries, l, entry)
     {
        if ((entry->size.min.w <= w) && (w <= entry->size.max.w))
          {
             if ((entry->size.min.h <= h) && (h <= entry->size.max.h))
               {
                  if (eps)
                    {
                       if (!*eps) *eps = calloc(1, sizeof(Edje_Real_Part_Set));
                       if (*eps)
                         {
                            (*eps)->entry = entry;
                            (*eps)->set = set;
                            (*eps)->id = id;
                         }
                    }
                  return entry->id;
               }
          }
     }
   
   return -1;
}

static void
_edje_real_part_image_set(Edje *ed, Edje_Real_Part *ep, FLOAT_T pos)
{
   int image_id;
   int image_count, image_num;

   image_id = _edje_image_find(ep->object, ed,
                               &ep->param1.set,
                               (Edje_Part_Description_Image*) ep->param1.description,
                               NULL);
   if (image_id < 0)
     {
        Edje_Image_Directory_Entry *ie;

        if (!ed->file->image_dir) ie = NULL;
        else ie = ed->file->image_dir->entries + (-image_id) - 1;
        if ((ie) &&
            (ie->source_type == EDJE_IMAGE_SOURCE_TYPE_EXTERNAL) &&
            (ie->entry))
          {
             evas_object_image_file_set(ep->object, ie->entry, NULL);
          }
     }
   else
     {
        image_count = 2;
        if (ep->param2)
          image_count += ((Edje_Part_Description_Image*) ep->param2->description)->image.tweens_count;
        image_num = TO_INT(MUL(pos, SUB(FROM_INT(image_count),
                                        FROM_DOUBLE(0.5))));
        if (image_num > (image_count - 1))
          image_num = image_count - 1;
        if (image_num <= 0)
          {
             image_id = _edje_image_find(ep->object, ed,
                                         &ep->param1.set,
                                         (Edje_Part_Description_Image*) ep->param1.description,
                                         NULL);
          }
        else
          if (ep->param2)
            {
               if (image_num == (image_count - 1))
                 {
                    image_id = _edje_image_find(ep->object, ed,
                                                &ep->param2->set,
                                                (Edje_Part_Description_Image*) ep->param2->description,
                                                NULL);
                 }
               else
                 {
                    Edje_Part_Image_Id *imid;

                    imid = ((Edje_Part_Description_Image*) ep->param2->description)->image.tweens[image_num - 1];
                    image_id = _edje_image_find(ep->object, ed, NULL, NULL, imid);
                 }
            }
        if (image_id < 0)
          {
             ERR("¨Part \"%s\" description, "
                 "\"%s\" %3.3f with image %i index has a missing image id in a set of %i !!!",
                 ep->part->name,
                 ep->param1.description->state.name,
                 ep->param1.description->state.value,
                 image_num,
                 image_count);
          }
        else
          {
             char buf[1024] = "edje/images/";

             /* Replace snprint("edje/images/%i") == memcpy + itoa */
             eina_convert_itoa(image_id, buf + 12); /* No need to check length as 2³² need only 10 characteres. */

             evas_object_image_file_set(ep->object, ed->file->path, buf);
             if (evas_object_image_load_error_get(ep->object) != EVAS_LOAD_ERROR_NONE)
               {
                  ERR("Error loading image collection \"%s\" from "
                      "file \"%s\". Missing EET Evas loader module?",
                      buf, ed->file->path);
                  switch (evas_object_image_load_error_get(ep->object))
                    {
                     case EVAS_LOAD_ERROR_GENERIC:
                        ERR("Error type: EVAS_LOAD_ERROR_GENERIC");
                        break;
                     case EVAS_LOAD_ERROR_DOES_NOT_EXIST:
                        ERR("Error type: EVAS_LOAD_ERROR_DOES_NOT_EXIST");
                        break;
                     case EVAS_LOAD_ERROR_PERMISSION_DENIED:
                        ERR("Error type: EVAS_LOAD_ERROR_PERMISSION_DENIED");
                        break;
                     case EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED:
                        ERR("Error type: EVAS_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED");
                        break;
                     case EVAS_LOAD_ERROR_CORRUPT_FILE:
                        ERR("Error type: EVAS_LOAD_ERROR_CORRUPT_FILE");
                        break;
                     case EVAS_LOAD_ERROR_UNKNOWN_FORMAT:
                        ERR("Error type: EVAS_LOAD_ERROR_UNKNOWN_FORMAT");
                        break;
                     default:
                        ERR("Error type: ???");
                        break;
                    }
               }
          }
     }
}

static void
_edje_real_part_rel_to_apply(Edje *ed, Edje_Real_Part *ep, Edje_Real_Part_State *state)
{
   state->rel1_to_x = state->rel1_to_y = NULL;
   state->rel2_to_x = state->rel2_to_y = NULL;

   if (state->description)
     {
        if (state->description->rel1.id_x >= 0)
          state->rel1_to_x = ed->table_parts[state->description->rel1.id_x % ed->table_parts_size];
        if (state->description->rel1.id_y >= 0)
          state->rel1_to_y = ed->table_parts[state->description->rel1.id_y % ed->table_parts_size];
        if (state->description->rel2.id_x >= 0)
          state->rel2_to_x = ed->table_parts[state->description->rel2.id_x % ed->table_parts_size];
        if (state->description->rel2.id_y >= 0)
          state->rel2_to_y = ed->table_parts[state->description->rel2.id_y % ed->table_parts_size];

        if (ep->part->type == EDJE_PART_TYPE_EXTERNAL)
          {
             Edje_Part_Description_External *external;

             if ((ep->type != EDJE_RP_TYPE_SWALLOW) ||
                 (!ep->typedata.swallow)) return;
             
             external = (Edje_Part_Description_External*)state->description;
             
             if (state->external_params)
               _edje_external_parsed_params_free(ep->typedata.swallow->swallowed_object, state->external_params);
             state->external_params = _edje_external_params_parse(ep->typedata.swallow->swallowed_object, external->external_params);
          }
     }
}

void
_edje_part_description_apply(Edje *ed, Edje_Real_Part *ep, const char *d1, double v1, const char *d2, double v2)
{
   Edje_Part_Description_Common *epd1;
   Edje_Part_Description_Common *epd2 = NULL;
   Edje_Part_Description_Common *chosen_desc;

   Edje_Part_Description_Image *epdi;

   if (!d1) d1 = "default";

   epd1 = _edje_part_description_find(ed, ep, d1, v1);
   if (!epd1)
     epd1 = ep->part->default_desc; /* never NULL */

   if (d2)
     epd2 = _edje_part_description_find(ed, ep, d2, v2);

   epdi = (Edje_Part_Description_Image*) epd2;

   /* There is an animation if both description are different or if description is an image with tweens */
   if (epd2 && (epd1 != epd2 || (ep->part->type == EDJE_PART_TYPE_IMAGE && epdi->image.tweens_count)))
     {
        if (!ep->param2)
          {
             ep->param2 = eina_mempool_malloc(_edje_real_part_state_mp,
                                              sizeof(Edje_Real_Part_State));
             memset(ep->param2, 0, sizeof(Edje_Real_Part_State));
          }
        else if (ep->part->type == EDJE_PART_TYPE_EXTERNAL)
          {
             if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
                 (ep->typedata.swallow))
               _edje_external_parsed_params_free(ep->typedata.swallow->swallowed_object,
                                                 ep->param2->external_params);
          }
        ep->param2->external_params = NULL;
     }
   else
     if (ep->param2)
       {
          if (ep->part->type == EDJE_PART_TYPE_EXTERNAL)
            {
               if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
                   (ep->typedata.swallow))
                 _edje_external_parsed_params_free(ep->typedata.swallow->swallowed_object,
                                                   ep->param2->external_params);
            }
          if (ep->param2)
            free(ep->param2->set);
          eina_mempool_free(_edje_real_part_state_mp, ep->param2);
          ep->param2 = NULL;
       }

   chosen_desc = ep->chosen_description;
   ep->param1.description = epd1;
   ep->chosen_description = epd1;

   _edje_real_part_rel_to_apply(ed, ep, &ep->param1);

   if (ep->param2)
     {
        ep->param2->description = epd2;

        _edje_real_part_rel_to_apply(ed, ep, ep->param2);

        if (ep->description_pos > FROM_DOUBLE(0.0))
          ep->chosen_description = epd2;
     }

   if (chosen_desc != ep->chosen_description &&
       ep->part->type == EDJE_PART_TYPE_EXTERNAL)
     _edje_external_recalc_apply(ed, ep, NULL, chosen_desc);

   ed->recalc_hints = EINA_TRUE;
   ed->dirty = EINA_TRUE;
   ed->recalc_call = EINA_TRUE;
#ifdef EDJE_CALC_CACHE
   ep->invalidate = 1;
#endif
}

void
_edje_recalc(Edje *ed)
{
   if ((ed->freeze > 0) || (_edje_freeze_val > 0))
     {
        ed->recalc = EINA_TRUE;
        if (!ed->calc_only)
          {
             if (_edje_freeze_val > 0)
               {
                  if (!ed->freeze_calc)
                    {
                       _edje_freeze_calc_count++;
                       _edje_freeze_calc_list = eina_list_append(_edje_freeze_calc_list, ed);
                       ed->freeze_calc = EINA_TRUE;
                    }
               }
             return;
          }
     }
// XXX: dont need this with current smart calc infra. remove me later
//   if (ed->postponed) return;
//   if (!ed->calc_only)
     evas_object_smart_changed(ed->obj);
// XXX: dont need this with current smart calc infra. remove me later
//   ed->postponed = EINA_TRUE;
}

void
_edje_recalc_do(Edje *ed)
{
   unsigned int i;
   Eina_Bool need_calc;

// XXX: dont need this with current smart calc infra. remove me later
//   ed->postponed = EINA_FALSE;
   need_calc = evas_object_smart_need_recalculate_get(ed->obj);
   evas_object_smart_need_recalculate_set(ed->obj, 0);
   if (!ed->dirty) return;
   ed->have_mapped_part = EINA_FALSE;
   ed->dirty = EINA_FALSE;
   ed->state++;
   for (i = 0; i < ed->table_parts_size; i++)
     {
        Edje_Real_Part *ep;

        ep = ed->table_parts[i];
        ep->calculated = FLAG_NONE;
        ep->calculating = FLAG_NONE;
     }
   for (i = 0; i < ed->table_parts_size; i++)
     {
        Edje_Real_Part *ep;

        ep = ed->table_parts[i];
        if (ep->calculated != FLAG_XY)
          _edje_part_recalc(ed, ep, (~ep->calculated) & FLAG_XY, NULL);
     }
   if (!ed->calc_only) ed->recalc = EINA_FALSE;
#ifdef EDJE_CALC_CACHE
   ed->all_part_change = EINA_FALSE;
   ed->text_part_change = EINA_FALSE;
#endif
   if (!ed->calc_only)
     {
        if (ed->recalc_call)
          evas_object_smart_callback_call(ed->obj, "recalc", NULL);
     }
   else
     evas_object_smart_need_recalculate_set(ed->obj, need_calc);
   ed->recalc_call = EINA_FALSE;

   if (ed->update_hints && ed->recalc_hints && !ed->calc_only)
     {
        Evas_Coord w, h;

        ed->recalc_hints = EINA_FALSE;

	eo_do(ed->obj, edje_obj_size_min_calc(&w, &h));
	eo_do(ed->obj, evas_obj_size_hint_min_set(w, h));
     }

   if (!ed->collection) return ;

   for (i = 0; i < ed->collection->limits.parts_count; i++)
     {
        const char *name;
        unsigned char limit;
        int part;

        part = ed->collection->limits.parts[i].part;
        name = ed->collection->parts[part]->name;
        limit = ed->table_parts[part]->chosen_description->limit;
        switch (limit)
          {
           case 0:
              ed->collection->limits.parts[i].width = EDJE_PART_LIMIT_UNKNOWN;
              ed->collection->limits.parts[i].height = EDJE_PART_LIMIT_UNKNOWN;
              break;
           case 1:
              ed->collection->limits.parts[i].height = EDJE_PART_LIMIT_UNKNOWN;
              break;
           case 2:
              ed->collection->limits.parts[i].width = EDJE_PART_LIMIT_UNKNOWN;
              break;
           case 3:
              break;
          }

        if ((limit & 1) == 1)
          {
             if (ed->table_parts[part]->w > 0 &&
                 (ed->collection->limits.parts[i].width != EDJE_PART_LIMIT_OVER))
               {
                  ed->collection->limits.parts[i].width = EDJE_PART_LIMIT_OVER;
                  _edje_emit(ed, "limit,width,over", name);
               }
             else if (ed->table_parts[part]->w < 0 &&
                      ed->collection->limits.parts[i].width != EDJE_PART_LIMIT_BELOW)
               {
                  ed->collection->limits.parts[i].width = EDJE_PART_LIMIT_BELOW;
                  _edje_emit(ed, "limit,width,below", name);
               }
             else if (ed->table_parts[part]->w == 0 &&
                      ed->collection->limits.parts[i].width != EDJE_PART_LIMIT_ZERO)
               {
                  ed->collection->limits.parts[i].width = EDJE_PART_LIMIT_ZERO;
                  _edje_emit(ed, "limit,width,zero", name);
               }
          }
        if ((limit & 2) == 2)
          {
             if (ed->table_parts[part]->h > 0 &&
                 (ed->collection->limits.parts[i].height != EDJE_PART_LIMIT_OVER))
               {
                  ed->collection->limits.parts[i].height = EDJE_PART_LIMIT_OVER;
                  _edje_emit(ed, "limit,height,over", name);
               }
             else if (ed->table_parts[part]->h < 0 &&
                      ed->collection->limits.parts[i].height != EDJE_PART_LIMIT_BELOW)
               {
                  ed->collection->limits.parts[i].height = EDJE_PART_LIMIT_BELOW;
                  _edje_emit(ed, "limit,height,below", name);
               }
             else if (ed->table_parts[part]->h == 0 &&
                      ed->collection->limits.parts[i].height != EDJE_PART_LIMIT_ZERO)
               {
                  ed->collection->limits.parts[i].height = EDJE_PART_LIMIT_ZERO;
                  _edje_emit(ed, "limit,height,zero", name);
               }
          }
     }
}

void
_edje_part_recalc_1(Edje *ed, Edje_Real_Part *ep)
{
  _edje_part_recalc(ed, ep, FLAG_XY, NULL);
}

int
_edje_part_dragable_calc(Edje *ed EINA_UNUSED, Edje_Real_Part *ep, FLOAT_T *x, FLOAT_T *y)
{
   if (ep->drag)
     {
        if (ep->drag->confine_to)
          {
             FLOAT_T dx, dy, dw, dh;
             int ret = 0;

             if ((ep->part->dragable.x != 0) &&
                 (ep->part->dragable.y != 0 )) ret = 3;
             else if (ep->part->dragable.x != 0) ret = 1;
             else if (ep->part->dragable.y != 0) ret = 2;

             dx = FROM_INT(ep->x - ep->drag->confine_to->x);
             dw = FROM_INT(ep->drag->confine_to->w - ep->w);
             if (dw != ZERO) dx = DIV(dx, dw);
             else dx = ZERO;

             dy = FROM_INT(ep->y - ep->drag->confine_to->y);
             dh = FROM_INT(ep->drag->confine_to->h - ep->h);
             if (dh != ZERO) dy = DIV(dy, dh);
             else dy = ZERO;

             if (x) *x = dx;
             if (y) *y = dy;

             return ret;
          }
        else
          {
             if (x) *x = ADD(FROM_INT(ep->drag->tmp.x), ep->drag->x);
             if (y) *y = ADD(FROM_INT(ep->drag->tmp.y), ep->drag->y);
             return 0;
          }
     }
   if (x) *x = ZERO;
   if (y) *y = ZERO;
   return 0;
}

void
_edje_dragable_pos_set(Edje *ed, Edje_Real_Part *ep, FLOAT_T x, FLOAT_T y)
{
   /* check whether this part is dragable at all */
   if (!ep->drag) return ;

   /* instead of checking for equality, we really should check that
    * the difference is greater than foo, but I have no idea what
    * value we would set foo to, because it would depend on the
    * size of the dragable...
    */
   if (ep->drag->x != x || ep->drag->tmp.x)
     {
        ep->drag->x = x;
        ep->drag->tmp.x = 0;
        ep->drag->need_reset = 0;
        ed->dirty = EINA_TRUE;
        ed->recalc_call = EINA_TRUE;
     }

   if (ep->drag->y != y || ep->drag->tmp.y)
     {
        ep->drag->y = y;
        ep->drag->tmp.y = 0;
        ep->drag->need_reset = 0;
        ed->dirty = EINA_TRUE;
        ed->recalc_call = EINA_TRUE;
     }

#ifdef EDJE_CALC_CACHE
   ep->invalidate = 1;
#endif
   _edje_recalc(ed); /* won't do anything if dirty flag isn't set */
}

static void
_edje_part_recalc_single_rel(Edje *ed,
                             Edje_Real_Part *ep EINA_UNUSED,
                             Edje_Part_Description_Common *desc,
                             Edje_Real_Part *rel1_to_x,
                             Edje_Real_Part *rel1_to_y,
                             Edje_Real_Part *rel2_to_x,
                             Edje_Real_Part *rel2_to_y,
                             Edje_Calc_Params *params)
{
   FLOAT_T x, w;
   FLOAT_T y, h;

   if (rel1_to_x)
     x = ADD(FROM_INT(desc->rel1.offset_x + rel1_to_x->x),
             SCALE(desc->rel1.relative_x, rel1_to_x->w));
   else
     x = ADD(FROM_INT(desc->rel1.offset_x),
             SCALE(desc->rel1.relative_x, ed->w));
   params->x = TO_INT(x);

   if (rel2_to_x)
     w = ADD(SUB(ADD(FROM_INT(desc->rel2.offset_x + rel2_to_x->x),
                     SCALE(desc->rel2.relative_x, rel2_to_x->w)),
                 x),
             FROM_INT(1));
   else
     w = ADD(SUB(ADD(FROM_INT(desc->rel2.offset_x),
                     SCALE(desc->rel2.relative_x, ed->w)),
                 x),
             FROM_INT(1));
   params->w = TO_INT(w);

   if (rel1_to_y)
     y = ADD(FROM_INT(desc->rel1.offset_y + rel1_to_y->y),
             SCALE(desc->rel1.relative_y, rel1_to_y->h));
   else
     y = ADD(FROM_INT(desc->rel1.offset_y),
             SCALE(desc->rel1.relative_y, ed->h));
   params->y = TO_INT(y);

   if (rel2_to_y)
     h = ADD(SUB(ADD(FROM_INT(desc->rel2.offset_y + rel2_to_y->y),
                     SCALE(desc->rel2.relative_y, rel2_to_y->h)),
                 y),
             FROM_INT(1));
   else
     h = ADD(SUB(ADD(FROM_INT(desc->rel2.offset_y),
                     SCALE(desc->rel2.relative_y, ed->h)),
                 y),
             FROM_INT(1));
   params->h = TO_INT(h);
}

static Edje_Internal_Aspect
_edje_part_recalc_single_aspect(Edje *ed,
                                Edje_Real_Part *ep,
                                Edje_Part_Description_Common *desc,
                                Edje_Calc_Params *params,
                                int *minw, int *minh,
                                int *maxw, int *maxh,
                                FLOAT_T pos)
{
   Edje_Internal_Aspect apref = EDJE_ASPECT_PREFER_NONE;
   FLOAT_T aspect, amax, amin;
   FLOAT_T new_w = ZERO, new_h = ZERO, want_x, want_y, want_w, want_h;

   if (params->h <= ZERO) aspect = FROM_INT(999999);
   else aspect = DIV(FROM_INT(params->w), FROM_INT(params->h));
   amax = desc->aspect.max;
   amin = desc->aspect.min;
   if (desc->aspect.prefer == EDJE_ASPECT_PREFER_SOURCE &&
       ep->part->type == EDJE_PART_TYPE_IMAGE)
     {
        Evas_Coord w, h;

        /* We only need pose to find the right image that would be displayed,
           and the right aspect ratio in that case */
        _edje_real_part_image_set(ed, ep, pos);
        evas_object_image_size_get(ep->object, &w, &h);
        amin = amax = DIV(FROM_INT(w), FROM_INT(h));
     }
   if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
       (ep->typedata.swallow))
     {
        if ((ep->typedata.swallow->swallow_params.aspect.w > 0) &&
            (ep->typedata.swallow->swallow_params.aspect.h > 0))
          amin = amax =
          DIV(FROM_INT(ep->typedata.swallow->swallow_params.aspect.w),
              FROM_INT(ep->typedata.swallow->swallow_params.aspect.h));
     }
   want_x = FROM_INT(params->x);
   want_w = new_w = FROM_INT(params->w);

   want_y = FROM_INT(params->y);
   want_h = new_h = FROM_INT(params->h);

   if ((amin > ZERO) && (amax > ZERO))
     {
        apref = desc->aspect.prefer;
        if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
            (ep->typedata.swallow))
          {
             if (ep->typedata.swallow->swallow_params.aspect.mode > EDJE_ASPECT_CONTROL_NONE)
               {
                  switch (ep->typedata.swallow->swallow_params.aspect.mode)
                    {
                     case EDJE_ASPECT_CONTROL_NEITHER:
                       apref = EDJE_ASPECT_PREFER_NONE;
                       break;
                     case EDJE_ASPECT_CONTROL_HORIZONTAL:
                       apref = EDJE_ASPECT_PREFER_HORIZONTAL;
                       break;
                     case EDJE_ASPECT_CONTROL_VERTICAL:
                       apref = EDJE_ASPECT_PREFER_VERTICAL;
                       break;
                     case EDJE_ASPECT_CONTROL_BOTH:
                       apref = EDJE_ASPECT_PREFER_BOTH;
                       break;
                     default:
                       break;
                    }
               }
          }
        switch (apref)
          {
           case EDJE_ASPECT_PREFER_NONE:
              /* keep both dimensions in check */
              /* adjust for min aspect (width / height) */
              if ((amin > ZERO) && (aspect < amin))
                {
                   new_h = DIV(FROM_INT(params->w), amin);
                   new_w = SCALE(amin, params->h);
                }
              /* adjust for max aspect (width / height) */
              if ((amax > ZERO) && (aspect > amax))
                {
                   new_h = DIV(FROM_INT(params->w), amax);
                   new_w = SCALE(amax, params->h);
                }
              if ((amax > ZERO) && (new_w < FROM_INT(params->w)))
                {
                   new_w = FROM_INT(params->w);
                   new_h = DIV(FROM_INT(params->w), amax);
                }
              if ((amax > ZERO) && (new_h < FROM_INT(params->h)))
                {
                   new_w = SCALE(amax, params->h);
                   new_h = FROM_INT(params->h);
                }
              break;
              /* prefer vertical size as determiner */
           case  EDJE_ASPECT_PREFER_VERTICAL:
              /* keep both dimensions in check */
              /* adjust for max aspect (width / height) */
              if ((amax > ZERO) && (aspect > amax))
                new_w = SCALE(amax, params->h);
              /* adjust for min aspect (width / height) */
              if ((amin > ZERO) && (aspect < amin))
                new_w = SCALE(amin, params->h);
              break;
              /* prefer horizontal size as determiner */
           case EDJE_ASPECT_PREFER_HORIZONTAL:
              /* keep both dimensions in check */
              /* adjust for max aspect (width / height) */
              if ((amax > ZERO) && (aspect > amax))
                new_h = DIV(FROM_INT(params->w), amax);
              /* adjust for min aspect (width / height) */
              if ((amin > ZERO) && (aspect < amin))
                new_h = DIV(FROM_INT(params->w), amin);
              break;
           case EDJE_ASPECT_PREFER_SOURCE:
           case EDJE_ASPECT_PREFER_BOTH:
              /* keep both dimensions in check */
              /* adjust for max aspect (width / height) */
              if ((amax > ZERO) && (aspect > amax))
                {
                   new_w = SCALE(amax, params->h);
                   new_h = DIV(FROM_INT(params->w), amax);
                }
              /* adjust for min aspect (width / height) */
              if ((amin > ZERO) && (aspect < amin))
                {
                   new_w = SCALE(amin, params->h);
                   new_h = DIV(FROM_INT(params->w), amin);
                }
              break;
           default:
              break;
          }

        if (!((amin > ZERO) && (amax > ZERO) &&
              (apref == EDJE_ASPECT_PREFER_NONE)))
          {
             if ((*maxw >= 0) && (new_w > FROM_INT(*maxw)))
               new_w = FROM_INT(*maxw);
             if (new_w < FROM_INT(*minw))
               new_w = FROM_INT(*minw);

             if ((FROM_INT(*maxh) >= 0) && (new_h > FROM_INT(*maxh)))
               new_h = FROM_INT(*maxh);
             if (new_h < FROM_INT(*minh))
               new_h = FROM_INT(*minh);
          }

        /* do real adjustment */
        if (apref == EDJE_ASPECT_PREFER_BOTH)
          {
             if (amin == ZERO) amin = amax;
             if (amin != ZERO)
               {
                  /* fix h and vary w */
                  if (new_w > FROM_INT(params->w))
                    {
                       //		  params->w = new_w;
                       // EXCEEDS BOUNDS in W
                       new_h = DIV(FROM_INT(params->w), amin);
                       new_w = FROM_INT(params->w);
                       if (new_h > FROM_INT(params->h))
                         {
                            new_h = FROM_INT(params->h);
                            new_w = SCALE(amin, params->h);
                         }
                    }
                  /* fix w and vary h */
                  else
                    {
                       //		  params->h = new_h;
                       // EXCEEDS BOUNDS in H
                       new_h = FROM_INT(params->h);
                       new_w = SCALE(amin, params->h);
                       if (new_w > FROM_INT(params->w))
                         {
                            new_h = DIV(FROM_INT(params->w), amin);
                            new_w = FROM_INT(params->w);
                         }
                    }
                  params->w = TO_INT(new_w);
                  params->h = TO_INT(new_h);
               }
          }
     }
   if (apref != EDJE_ASPECT_PREFER_BOTH)
     {
        if ((amin > 0.0) && (amax > ZERO) && (apref == EDJE_ASPECT_PREFER_NONE))
          {
             params->w = TO_INT(new_w);
             params->h = TO_INT(new_h);
          }
        else if ((FROM_INT(params->h) - new_h) > (FROM_INT(params->w) - new_w))
          {
             if (params->h < TO_INT(new_h))
               params->h = TO_INT(new_h);
             else if (params->h > TO_INT(new_h))
               params->h = TO_INT(new_h);
             if (apref == EDJE_ASPECT_PREFER_VERTICAL)
               params->w = TO_INT(new_w);
          }
        else
          {
             if (params->w < TO_INT(new_w))
               params->w = TO_INT(new_w);
             else if (params->w > TO_INT(new_w))
               params->w = TO_INT(new_w);
             if (apref == EDJE_ASPECT_PREFER_HORIZONTAL)
               params->h = TO_INT(new_h);
          }
     }
   params->x = TO_INT(ADD(want_x,
                          MUL(SUB(want_w, FROM_INT(params->w)),
                              desc->align.x)));
   params->y = TO_INT(ADD(want_y,
                          MUL(SUB(want_h, FROM_INT(params->h)),
                              desc->align.y)));
   return apref;
}

static void
_edje_part_recalc_single_step(Edje_Part_Description_Common *desc,
                              Edje_Calc_Params *params)
{
   if (desc->step.x > 0)
     {
        int steps;
        int new_w;

        steps = params->w / desc->step.x;
        new_w = desc->step.x * steps;
        if (params->w > new_w)
          {
             params->x += TO_INT(SCALE(desc->align.x, (params->w - new_w)));
             params->w = new_w;
          }
     }

   if (desc->step.y > 0)
     {
        int steps;
        int new_h;

        steps = params->h / desc->step.y;
        new_h = desc->step.y * steps;
        if (params->h > new_h)
          {
             params->y += TO_INT(SCALE(desc->align.y, (params->h - new_h)));
             params->h = new_h;
          }
     }
}

static void
_edje_part_recalc_single_textblock(FLOAT_T sc,
                                   Edje *ed,
                                   Edje_Real_Part *ep,
                                   Edje_Part_Description_Text *chosen_desc,
                                   Edje_Calc_Params *params,
                                   int *minw, int *minh,
                                   int *maxw, int *maxh)
{
   if ((ep->type != EDJE_RP_TYPE_TEXT) ||
       (!ep->typedata.text))
     return;
   if (chosen_desc)
     {
        Evas_Coord tw, th, ins_l, ins_r, ins_t, ins_b;
        const char *text = "";
        const char *style = "";
        Edje_Style *stl  = NULL;
        const char *tmp;
        Eina_List *l;

        if (chosen_desc->text.id_source >= 0)
          {
             ep->typedata.text->source = ed->table_parts[chosen_desc->text.id_source % ed->table_parts_size];

             tmp = edje_string_get(&((Edje_Part_Description_Text *)ep->typedata.text->source->chosen_description)->text.style);
             if (tmp) style = tmp;
          }
        else
          {
             ep->typedata.text->source = NULL;

             tmp = edje_string_get(&chosen_desc->text.style);
             if (tmp) style = tmp;
          }

        if (chosen_desc->text.id_text_source >= 0)
          {
             ep->typedata.text->text_source = ed->table_parts[chosen_desc->text.id_text_source % ed->table_parts_size];
             text = edje_string_get(&((Edje_Part_Description_Text*)ep->typedata.text->text_source->chosen_description)->text.text);

             if (ep->typedata.text->text_source->typedata.text->text) text = ep->typedata.text->text_source->typedata.text->text;
          }
        else
          {
             ep->typedata.text->text_source = NULL;
             text = edje_string_get(&chosen_desc->text.text);
             if (ep->typedata.text->text) text = ep->typedata.text->text;
          }

        EINA_LIST_FOREACH(ed->file->styles, l, stl)
          {
             if ((stl->name) && (!strcmp(stl->name, style))) break;
             stl = NULL;
          }

        if (ep->part->scale)
          evas_object_scale_set(ep->object, TO_DOUBLE(sc));

        if (stl)
          {
             const char *ptxt;

             if (evas_object_textblock_style_get(ep->object) != stl->style)
               evas_object_textblock_style_set(ep->object, stl->style);
             // FIXME: need to account for editing
             if (ep->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
               {
                  // do nothing - should be done elsewhere
               }
             else
               {
                  ptxt = evas_object_textblock_text_markup_get(ep->object);
                  if (((!ptxt) && (text)) ||
                      ((ptxt) && (text) && (strcmp(ptxt, text))) ||
                      ((ptxt) && (!text)))
                    evas_object_textblock_text_markup_set(ep->object, text);
               }
             if ((chosen_desc->text.min_x) || (chosen_desc->text.min_y))
               {
                  int mw = 0, mh = 0;

                  tw = th = 0;
                  if (!chosen_desc->text.min_x)
                    {
		       eo_do(ep->object,
			     evas_obj_size_set(params->w, params->h),
			     evas_obj_textblock_size_formatted_get(&tw, &th));
                    }
                  else
                    evas_object_textblock_size_native_get(ep->object, &tw, &th);
                  evas_object_textblock_style_insets_get(ep->object, &ins_l,
                                                         &ins_r, &ins_t, &ins_b);
                  mw = ins_l + tw + ins_r;
                  mh = ins_t + th + ins_b;
                  if (minw && chosen_desc->text.min_x)
                    {
                       if (mw > *minw) *minw = mw;
                    }
                  if (minh && chosen_desc->text.min_y)
                    {
                       if (mh > *minh) *minh = mh;
                    }
               }
          }
        if ((chosen_desc->text.max_x) || (chosen_desc->text.max_y))
          {
             int mw = 0, mh = 0;

             tw = th = 0;
             if (!chosen_desc->text.max_x)
               {
		  eo_do(ep->object,
			evas_obj_size_set(params->w, params->h),
			evas_obj_textblock_size_formatted_get(&tw, &th));
               }
             else
               evas_object_textblock_size_native_get(ep->object, &tw, &th);
             evas_object_textblock_style_insets_get(ep->object, &ins_l, &ins_r,
                                                    &ins_t, &ins_b);
             mw = ins_l + tw + ins_r;
             mh = ins_t + th + ins_b;
             if (maxw && chosen_desc->text.max_x)
               {
                  if (mw > *maxw) *maxw = mw;
                  if (minw && (*maxw < *minw)) *maxw = *minw;
               }
             if (maxh && chosen_desc->text.max_y)
               {
                  if (mh > *maxh) *maxh = mh;
                  if (minh && (*maxh < *minh)) *maxh = *minh;
               }
          }
        if ((chosen_desc->text.fit_x) || (chosen_desc->text.fit_y))
          {
             double s = 1.0;

             if (ep->part->scale) s = TO_DOUBLE(sc);
	     eo_do(ep->object,
		   evas_obj_scale_set(s),
		   evas_obj_textblock_size_formatted_get(&tw, &th));
             if (chosen_desc->text.fit_x)
               {
                  if ((tw > 0) && (tw > params->w))
                    {
                       s = (s * params->w) / (double)tw;
		       eo_do(ep->object,
			     evas_obj_scale_set(s),
			     evas_obj_textblock_size_formatted_get(&tw, &th));
                    }
               }
             if (chosen_desc->text.fit_y)
               {
                  if ((th > 0) && (th > params->h))
                    {
                       s = (s * params->h) / (double)th;
		       eo_do(ep->object,
			     evas_obj_scale_set(s),
			     evas_obj_textblock_size_formatted_get(&tw, &th));
                    }
               }
          }
        evas_object_textblock_valign_set(ep->object, TO_DOUBLE(chosen_desc->text.align.y));
     }
}

static void
_edje_textblock_recalc_apply(Edje *ed, Edje_Real_Part *ep,
			Edje_Calc_Params *params,
			Edje_Part_Description_Text *chosen_desc)
{
   /* FIXME: this is just an hack. */
   FLOAT_T sc;
   sc = ed->scale;
   if (sc == ZERO) sc = _edje_scale;
   if (chosen_desc->text.fit_x || chosen_desc->text.fit_y)
     {
        _edje_part_recalc_single_textblock(sc, ed, ep, chosen_desc, params,
              NULL, NULL, NULL, NULL);
     }
}

static void
_edje_part_recalc_single_text(FLOAT_T sc EINA_UNUSED,
                              Edje *ed,
                              Edje_Real_Part *ep,
                              Edje_Part_Description_Text *desc,
                              Edje_Part_Description_Text *chosen_desc,
                              Edje_Calc_Params *params,
                              int *minw, int *minh,
                              int *maxw, int *maxh)
#define RECALC_SINGLE_TEXT_USING_APPLY 1
#if RECALC_SINGLE_TEXT_USING_APPLY
/*
 * XXX TODO NOTE:
 *
 * Original _edje_part_recalc_single_text() was not working as
 * expected since it was not doing size fit, range, ellipsis and so
 * on.
 *
 * The purpose of this function compared with
 * _edje_text_recalc_apply() is to be faster, not calling Evas update
 * functions. However for text this is quite difficult given that to
 * fit we need to set the font, size, style, etc. If it was done
 * correctly, we'd save some calls to move and some color sets,
 * however those shouldn't matter much in the overall picture.
 *
 * I've changed this to force applying the value, it should be more
 * correct and not so slow. The previous code is kept below for
 * reference but should be removed before next release!
 *
 * -- Gustavo Barbieri at 20-Aug-2011
 */
{
   int tw, th, mw, mh, l, r, t, b, size;
   char *sfont = NULL;

   _edje_text_class_font_get(ed, desc, &size, &sfont);
   free(sfont);
   params->type.text.size = size; /* XXX TODO used by further calcs, go inside recalc_apply? */

   _edje_text_recalc_apply(ed, ep, params, chosen_desc);

   if ((!chosen_desc) ||
       ((!chosen_desc->text.min_x) && (!chosen_desc->text.min_y) &&
        (!chosen_desc->text.max_x) && (!chosen_desc->text.max_y)))
     return;

   eo_do(ep->object,
	 evas_obj_size_get(&tw, &th),
	 evas_obj_text_style_pad_get(&l, &r, &t, &b));

   mw = tw + l + r;
   mh = th + t + b;

   if (chosen_desc->text.max_x)
     {
        if ((*maxw < 0) || (mw < *maxw)) *maxw = mw;
     }
   if (chosen_desc->text.max_y)
     {
        if ((*maxh < 0) || (mh < *maxh)) *maxh = mh;
     }
   if (chosen_desc->text.min_x)
     {
        if (mw > *minw) *minw = mw;
     }
   if (chosen_desc->text.min_y)
     {
        if (mh > *minh) *minh = mh;
     }
}
#else
{
   char *sfont = NULL;
   int size;

   if (chosen_desc)
     {
        const char *text;
        const char *font;
        Evas_Coord tw, th;
        int inlined_font = 0;

        /* Update a object_text part */

        if (chosen_desc->text.id_source >= 0)
          ep->text.source = ed->table_parts[chosen_desc->text.id_source % ed->table_parts_size];
        else
          ep->text.source = NULL;

        if (chosen_desc->text.id_text_source >= 0)
          ep->text.text_source = ed->table_parts[chosen_desc->text.id_text_source % ed->table_parts_size];
        else
          ep->text.text_source = NULL;

        if (ep->text.text_source)
          text = edje_string_get(&(((Edje_Part_Description_Text*)ep->text.text_source->chosen_description)->text.text));
        else
          text = edje_string_get(&chosen_desc->text.text);

        if (ep->text.source)
          font = _edje_text_class_font_get(ed, ((Edje_Part_Description_Text*)ep->text.source->chosen_description), &size, &sfont);
        else
          font = _edje_text_class_font_get(ed, chosen_desc, &size, &sfont);

        if (!font) font = "";

        if (ep->text.text_source)
          {
             if (ep->text.text_source->text.text) text = ep->text.text_source->text.text;
          }
        else
          {
             if (ep->text.text) text = ep->text.text;
          }

        if (ep->text.source)
          {
             if (ep->text.source->text.font) font = ep->text.source->text.font;
             if (ep->text.source->text.size > 0) size = ep->text.source->text.size;
          }
        else
          {
             if (ep->text.font) font = ep->text.font;
             if (ep->text.size > 0) size = ep->text.size;
          }
        if (!text) text = "";

        /* check if the font is embedded in the .eet */
        if (ed->file->fonts)
          {
             Edje_Font_Directory_Entry *fnt;

             fnt = eina_hash_find(ed->file->fonts, font);

             if (fnt)
               {
                  char *font2;

                  size_t len = strlen(font) + sizeof("edje/fonts/") + 1;
                  font2 = alloca(len);
                  sprintf(font2, "edje/fonts/%s", font);
                  font = font2;
                  inlined_font = 1;
               }
          }
        if (ep->part->scale)
          evas_object_scale_set(ep->object, TO_DOUBLE(sc));
        if (inlined_font)
          {
             evas_object_text_font_source_set(ep->object, ed->path);
          }
        else evas_object_text_font_source_set(ep->object, NULL);

        if ((_edje_fontset_append) && (font))
          {
             char *font2;

             font2 = malloc(strlen(font) + 1 + strlen(_edje_fontset_append) + 1);
             if (font2)
               {
                  strcpy(font2, font);
                  strcat(font2, ",");
                  strcat(font2, _edje_fontset_append);
                  evas_object_text_font_set(ep->object, font2, size);
                  free(font2);
               }
          }
        else
          evas_object_text_font_set(ep->object, font, size);
        if ((chosen_desc->text.min_x) || (chosen_desc->text.min_y) ||
            (chosen_desc->text.max_x) || (chosen_desc->text.max_y))
          {
             int mw, mh;
             Evas_Text_Style_Type
                style = EVAS_TEXT_STYLE_PLAIN,
                      shadow = EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT;
             const Evas_Text_Style_Type styles[] = {
                  EVAS_TEXT_STYLE_PLAIN,
                  EVAS_TEXT_STYLE_PLAIN,
                  EVAS_TEXT_STYLE_OUTLINE,
                  EVAS_TEXT_STYLE_SOFT_OUTLINE,
                  EVAS_TEXT_STYLE_SHADOW,
                  EVAS_TEXT_STYLE_SOFT_SHADOW,
                  EVAS_TEXT_STYLE_OUTLINE_SHADOW,
                  EVAS_TEXT_STYLE_OUTLINE_SOFT_SHADOW,
                  EVAS_TEXT_STYLE_FAR_SHADOW,
                  EVAS_TEXT_STYLE_FAR_SOFT_SHADOW,
                  EVAS_TEXT_STYLE_GLOW
             };
             const Evas_Text_Style_Type shadows[] = {
                  EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_RIGHT,
                  EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM,
                  EVAS_TEXT_STYLE_SHADOW_DIRECTION_BOTTOM_LEFT,
                  EVAS_TEXT_STYLE_SHADOW_DIRECTION_LEFT,
                  EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_LEFT,
                EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP,
                EVAS_TEXT_STYLE_SHADOW_DIRECTION_TOP_RIGHT,
                EVAS_TEXT_STYLE_SHADOW_DIRECTION_RIGHT
             };

             if ((ep->part->effect & EVAS_TEXT_STYLE_MASK_BASIC)
                 < EDJE_TEXT_EFFECT_LAST)
               style = styles[ep->part->effect];
             shadow = shadows
                [(ep->part->effect & EDJE_TEXT_EFFECT_MASK_SHADOW_DIRECTION) >> 4];
             EVAS_TEXT_STYLE_SHADOW_DIRECTION_SET(style, shadow);

	     eo_do(ep->object,
		   evas_obj_text_style_set(style),
		   evas_obj_text_text_set(text),
		   evas_obj_size_get(&tw, &th));
             if (chosen_desc->text.max_x)
               {
                  int l, r;
                  evas_object_text_style_pad_get(ep->object, &l, &r, NULL, NULL);
                  mw = tw + l + r;
                  if ((*maxw < 0) || (mw < *maxw)) *maxw = mw;
               }
             if (chosen_desc->text.max_y)
               {
                  int t, b;
                  evas_object_text_style_pad_get(ep->object, NULL, NULL, &t, &b);
                  mh = th + t + b;
                  if ((*maxh < 0) || (mh < *maxh)) *maxh = mh;
               }
             if (chosen_desc->text.min_x)
               {
                  int l, r;
                  evas_object_text_style_pad_get(ep->object, &l, &r, NULL, NULL);
                  mw = tw + l + r;
                  if (mw > *minw) *minw = mw;
               }
             if (chosen_desc->text.min_y)
               {
                  int t, b;
                  evas_object_text_style_pad_get(ep->object, NULL, NULL, &t, &b);
                  mh = th + t + b;
                  if (mh > *minh) *minh = mh;
               }
          }
        if (sfont) free(sfont);
     }

   /* FIXME: Do we really need to call it twice if chosen_desc ? */
   sfont = NULL;
   _edje_text_class_font_get(ed, desc, &size, &sfont);
   free(sfont);
   params->type.text.size = size;
}
#endif

static void
_edje_part_recalc_single_min_length(FLOAT_T align, int *start, int *length, int min)
{
   if (min >= 0)
     {
        if (*length < min)
          {
             *start += TO_INT(SCALE(align, (*length - min)));
             *length = min;
          }
     }
}

static void
_edje_part_recalc_single_min(Edje_Part_Description_Common *desc,
                             Edje_Calc_Params *params,
                             int minw, int minh,
                             Edje_Internal_Aspect aspect)
{
   int tmp;
   int w;
   int h;

   w = params->w ? params->w : 99999;
   h = params->h ? params->h : 99999;

   switch (aspect)
     {
      case EDJE_ASPECT_PREFER_NONE:
         break;
      case EDJE_ASPECT_PREFER_VERTICAL:
         tmp = minh * params->w / h;
         if (tmp >= minw)
           {
              minw = tmp;
              break;
           }
      case EDJE_ASPECT_PREFER_HORIZONTAL:
         tmp = minw * params->h / w;
         if (tmp >= minh)
           {
              minh = tmp;
              break;
           }
      case EDJE_ASPECT_PREFER_SOURCE:
      case EDJE_ASPECT_PREFER_BOTH:
         tmp = minh * params->w / h;
         if (tmp >= minw)
           {
              minw = tmp;
              break;
           }

         tmp = minw * params->h / w;
         if (tmp >= minh)
           {
              minh = tmp;
              break;
           }

         break;
     }

   _edje_part_recalc_single_min_length(desc->align.x, &params->x, &params->w, minw);
   _edje_part_recalc_single_min_length(desc->align.y, &params->y, &params->h, minh);
}

static void
_edje_part_recalc_single_max_length(FLOAT_T align, int *start, int *length, int max)
{
   if (max >= 0)
     {
        if (*length > max)
          {
             *start += TO_INT(SCALE(align, (*length - max)));
             *length = max;
          }
     }
}

static void
_edje_part_recalc_single_max(Edje_Part_Description_Common *desc,
                             Edje_Calc_Params *params,
                             int maxw, int maxh,
                             Edje_Internal_Aspect aspect)
{
   int tmp;
   int w;
   int h;

   w = params->w ? params->w : 99999;
   h = params->h ? params->h : 99999;

   switch (aspect)
     {
      case EDJE_ASPECT_PREFER_NONE:
         break;
      case EDJE_ASPECT_PREFER_VERTICAL:
         tmp = maxh * params->w / h;
         if (tmp <= maxw)
           {
              maxw = tmp;
              break;
           }
      case EDJE_ASPECT_PREFER_HORIZONTAL:
         tmp = maxw * params->h / w;
         if (tmp <= maxh)
           {
              maxh = tmp;
              break;
           }
      case EDJE_ASPECT_PREFER_SOURCE:
      case EDJE_ASPECT_PREFER_BOTH:
         tmp = maxh * params->w / h;
         if (tmp <= maxw)
           {
              maxw = tmp;
              break;
           }

         tmp = maxw * params->h / w;
         if (tmp <= maxh)
           {
              maxh = tmp;
              break;
           }

         break;
     }

   _edje_part_recalc_single_max_length(desc->align.x, &params->x, &params->w, maxw);
   _edje_part_recalc_single_max_length(desc->align.y, &params->y, &params->h, maxh);
}

static void
_edje_part_recalc_single_drag(Edje_Real_Part *ep,
                              Edje_Real_Part *confine_to,
                              Edje_Calc_Params *params,
                              int minw, int minh,
                              int maxw, int maxh)
{
   /* confine */
   if (confine_to)
     {
        int offset;
        int step;
        FLOAT_T v;

        /* complex dragable params */
        v = SCALE(ep->drag->size.x, confine_to->w);

        if ((minw > 0) && (TO_INT(v) < minw)) params->w = minw;
        else if ((maxw >= 0) && (TO_INT(v) > maxw)) params->w = maxw;
        else params->w = TO_INT(v);

        offset = TO_INT(SCALE(ep->drag->x, (confine_to->w - params->w)))
           + ep->drag->tmp.x;
        if (ep->part->dragable.step_x > 0)
          {
             params->x = confine_to->x +
                ((offset / ep->part->dragable.step_x) * ep->part->dragable.step_x);
          }
        else if (ep->part->dragable.count_x > 0)
          {
             step = (confine_to->w - params->w) / ep->part->dragable.count_x;
             if (step < 1) step = 1;
             params->x = confine_to->x +
                ((offset / step) * step);
          }
        params->req_drag.x = params->x;
        params->req_drag.w = params->w;

        v = SCALE(ep->drag->size.y, confine_to->h);

        if ((minh > 0) && (TO_INT(v) < minh)) params->h = minh;
        else if ((maxh >= 0) && (TO_INT(v) > maxh)) params->h = maxh;
        else params->h = TO_INT(v);

        offset = TO_INT(SCALE(ep->drag->y, (confine_to->h - params->h)))
           + ep->drag->tmp.y;
        if (ep->part->dragable.step_y > 0)
          {
             params->y = confine_to->y +
                ((offset / ep->part->dragable.step_y) * ep->part->dragable.step_y);
          }
        else if (ep->part->dragable.count_y > 0)
          {
             step = (confine_to->h - params->h) / ep->part->dragable.count_y;
             if (step < 1) step = 1;
             params->y = confine_to->y +
                ((offset / step) * step);
          }
        params->req_drag.y = params->y;
        params->req_drag.h = params->h;

        /* limit to confine */
        if (params->x < confine_to->x)
          {
             params->x = confine_to->x;
          }
        if ((params->x + params->w) > (confine_to->x + confine_to->w))
          {
             params->x = confine_to->x + confine_to->w - params->w;
          }
        if (params->y < confine_to->y)
          {
             params->y = confine_to->y;
          }
        if ((params->y + params->h) > (confine_to->y + confine_to->h))
          {
             params->y = confine_to->y + confine_to->h - params->h;
          }
     }
   else
     {
        /* simple dragable params */
        params->x += TO_INT(ep->drag->x) + ep->drag->tmp.x;
        params->req_drag.x = params->x;
        params->req_drag.w = params->w;

        params->y += TO_INT(ep->drag->y) + ep->drag->tmp.y;
        params->req_drag.y = params->y;
        params->req_drag.h = params->h;
     }
}

static void
_edje_part_recalc_single_fill(Edje_Real_Part *ep,
                              Edje_Part_Description_Spec_Fill *fill,
                              Edje_Calc_Params *params)
{
   int fw;
   int fh;

   params->smooth = fill->smooth;

   if (fill->type == EDJE_FILL_TYPE_TILE)
     evas_object_image_size_get(ep->object, &fw, NULL);
   else
     fw = params->w;

   params->type.common.fill.x = fill->pos_abs_x
      + TO_INT(SCALE(fill->pos_rel_x, fw));
   params->type.common.fill.w = fill->abs_x
      + TO_INT(SCALE(fill->rel_x, fw));

   if (fill->type == EDJE_FILL_TYPE_TILE)
     evas_object_image_size_get(ep->object, NULL, &fh);
   else
     fh = params->h;

   params->type.common.fill.y = fill->pos_abs_y
      + TO_INT(SCALE(fill->pos_rel_y, fh));
   params->type.common.fill.h = fill->abs_y
      + TO_INT(SCALE(fill->rel_y, fh));

   params->type.common.fill.angle = fill->angle;
   params->type.common.fill.spread = fill->spread;
}

static void
_edje_part_recalc_single_min_max(FLOAT_T sc,
                                 Edje_Real_Part *ep,
                                 Edje_Part_Description_Common *desc,
                                 int *minw, int *minh,
                                 int *maxw, int *maxh)
{
   *minw = desc->min.w;
   if (ep->part->scale) *minw = TO_INT(SCALE(sc, *minw));
   if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
       (ep->typedata.swallow))
     {
        if (ep->typedata.swallow->swallow_params.min.w > desc->min.w)
          *minw = ep->typedata.swallow->swallow_params.min.w;
     }

   if (ep->edje->calc_only)
     {
        if (desc->minmul.have)
          {
             FLOAT_T mmw = desc->minmul.w;
             if (mmw != FROM_INT(1)) *minw = TO_INT(SCALE(mmw, *minw));
          }
     }

   if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
       (ep->typedata.swallow))
     {
        /* XXX TODO: remove need of EDJE_INF_MAX_W, see edje_util.c */
        if ((ep->typedata.swallow->swallow_params.max.w <= 0) ||
            (ep->typedata.swallow->swallow_params.max.w == EDJE_INF_MAX_W))
          {
             *maxw = desc->max.w;
             if (*maxw > 0)
               {
                  if (ep->part->scale) *maxw = TO_INT(SCALE(sc, *maxw));
                  if (*maxw < 1) *maxw = 1;
               }
          }
        else
          {
             if (desc->max.w <= 0)
               *maxw = ep->typedata.swallow->swallow_params.max.w;
             else
               {
                  *maxw = desc->max.w;
                  if (*maxw > 0)
                    {
                       if (ep->part->scale) *maxw = TO_INT(SCALE(sc, *maxw));
                       if (*maxw < 1) *maxw = 1;
                    }
                  if (ep->typedata.swallow->swallow_params.max.w < *maxw)
                    *maxw = ep->typedata.swallow->swallow_params.max.w;
               }
          }
     }
   else
     {
        *maxw = desc->max.w;
        if (*maxw > 0)
          {
             if (ep->part->scale) *maxw = TO_INT(SCALE(sc, *maxw));
             if (*maxw < 1) *maxw = 1;
          }
     }
   if ((ep->edje->calc_only) && (desc->minmul.have) &&
       (desc->minmul.w != FROM_INT(1))) *maxw = *minw;
   if (*maxw >= 0)
     {
        if (*maxw < *minw) *maxw = *minw;
     }

   *minh = desc->min.h;
   if (ep->part->scale) *minh = TO_INT(SCALE(sc, *minh));
   if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
       (ep->typedata.swallow))
     {
        if (ep->typedata.swallow->swallow_params.min.h > desc->min.h)
          *minh = ep->typedata.swallow->swallow_params.min.h;
     }

   if (ep->edje->calc_only)
     {
        if (desc->minmul.have)
          {
             FLOAT_T mmh = desc->minmul.h;
             if (mmh != FROM_INT(1)) *minh = TO_INT(SCALE(mmh, *minh));
          }
     }

   if ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
       (ep->typedata.swallow))
     {
        /* XXX TODO: remove need of EDJE_INF_MAX_H, see edje_util.c */
        if ((ep->typedata.swallow->swallow_params.max.h <= 0) ||
            (ep->typedata.swallow->swallow_params.max.h == EDJE_INF_MAX_H))
          {
             *maxh = desc->max.h;
             if (*maxh > 0)
               {
                  if (ep->part->scale) *maxh = TO_INT(SCALE(sc, *maxh));
                  if (*maxh < 1) *maxh = 1;
               }
          }
        else
          {
             if (desc->max.h <= 0)
               *maxh = ep->typedata.swallow->swallow_params.max.h;
             else
               {
                  *maxh = desc->max.h;
                  if (*maxh > 0)
                    {
                       if (ep->part->scale) *maxh = TO_INT(SCALE(sc, *maxh));
                       if (*maxh < 1) *maxh = 1;
                    }
                  if (ep->typedata.swallow->swallow_params.max.h < *maxh)
                    *maxh = ep->typedata.swallow->swallow_params.max.h;
               }
          }
     }
   else
     {
        *maxh = desc->max.h;
        if (*maxh > 0)
          {
             if (ep->part->scale) *maxh = TO_INT(SCALE(sc, *maxh));
             if (*maxh < 1) *maxh = 1;
          }
     }
   if ((ep->edje->calc_only) && (desc->minmul.have) &&
       (desc->minmul.h != FROM_INT(1))) *maxh = *minh;
   if (*maxh >= 0)
     {
        if (*maxh < *minh) *maxh = *minh;
     }
}

static void
_edje_part_recalc_single_map(Edje *ed,
                             Edje_Real_Part *ep EINA_UNUSED,
                             Edje_Real_Part *center,
                             Edje_Real_Part *light,
                             Edje_Real_Part *persp,
                             Edje_Part_Description_Common *desc,
                             Edje_Part_Description_Common *chosen_desc,
                             Edje_Calc_Params *params)
{
   params->mapped = chosen_desc->map.on;
   params->lighted = params->mapped ? !!light : 0;
   params->persp_on = params->mapped ? !!persp : 0;

   if (!params->mapped) return ;

   if (center)
     {
        params->map.center.x = ed->x + center->x + (center->w / 2);
        params->map.center.y = ed->y + center->y + (center->h / 2);
     }
   else
     {
        params->map.center.x = ed->x + params->x + (params->w / 2);
        params->map.center.y = ed->y + params->y + (params->h / 2);
     }
   params->map.center.z = 0;

   params->map.rotation.x = desc->map.rot.x;
   params->map.rotation.y = desc->map.rot.y;
   params->map.rotation.z = desc->map.rot.z;

   if (light)
     {
        Edje_Part_Description_Common *light_desc2;
        FLOAT_T pos, pos2;

        params->map.light.x = ed->x + light->x + (light->w / 2);
        params->map.light.y = ed->y + light->y + (light->h / 2);

        pos = light->description_pos;
        pos2 = (pos < ZERO) ? ZERO : ((pos > FROM_INT(1)) ? FROM_INT(1) : pos);

        light_desc2 = light->param2 ? light->param2->description : NULL;

        /* take into account CURRENT state also */
        if (pos != ZERO && light_desc2)
          {
             params->map.light.z = light->param1.description->persp.zplane +
               TO_INT(SCALE(pos, light_desc2->persp.zplane - light->param1.description->persp.zplane));
             params->map.light.r = light->param1.description->color.r +
               TO_INT(SCALE(pos2, light_desc2->color.r - light->param1.description->color.r));
             params->map.light.g = light->param1.description->color.g +
               TO_INT(SCALE(pos2, light_desc2->color.g - light->param1.description->color.g));
             params->map.light.b = light->param1.description->color.b +
               TO_INT(SCALE(pos2, light_desc2->color.b - light->param1.description->color.b));
             params->map.light.ar = light->param1.description->color2.r +
               TO_INT(SCALE(pos2, light_desc2->color2.r - light->param1.description->color2.r));
             params->map.light.ag = light->param1.description->color2.g +
               TO_INT(SCALE(pos2, light_desc2->color2.g - light->param1.description->color2.g));
             params->map.light.ab = light->param1.description->color2.b +
               TO_INT(SCALE(pos2, light_desc2->color2.b - light->param1.description->color2.b));
          }
        else
          {
             params->map.light.z = light->param1.description->persp.zplane;
             params->map.light.r = light->param1.description->color.r;
             params->map.light.g = light->param1.description->color.g;
             params->map.light.b = light->param1.description->color.b;
             params->map.light.ar = light->param1.description->color2.r;
             params->map.light.ag = light->param1.description->color2.g;
             params->map.light.ab = light->param1.description->color2.b;
          }
     }

   if (persp)
     {
        FLOAT_T pos;

        params->map.persp.x = ed->x + persp->x + (persp->w / 2);
        params->map.persp.y = ed->y + persp->y + (persp->h / 2);

        pos = persp->description_pos;

        if (pos != 0 && persp->param2)
          {
             params->map.persp.z = persp->param1.description->persp.zplane +
               TO_INT(SCALE(pos, persp->param2->description->persp.zplane -
                            persp->param1.description->persp.zplane));
             params->map.persp.focal = persp->param1.description->persp.focal +
               TO_INT(SCALE(pos, persp->param2->description->persp.focal -
                            persp->param1.description->persp.focal));
          }
        else
          {
             params->map.persp.z = persp->param1.description->persp.zplane;
             params->map.persp.focal = persp->param1.description->persp.focal;
          }
     }
}

static void
_edje_part_recalc_single(Edje *ed,
                         Edje_Real_Part *ep,
                         Edje_Part_Description_Common *desc,
                         Edje_Part_Description_Common *chosen_desc,
                         Edje_Real_Part *center,
                         Edje_Real_Part *light,
                         Edje_Real_Part *persp,
                         Edje_Real_Part *rel1_to_x,
                         Edje_Real_Part *rel1_to_y,
                         Edje_Real_Part *rel2_to_x,
                         Edje_Real_Part *rel2_to_y,
                         Edje_Real_Part *confine_to,
                         Edje_Calc_Params *params,
                         FLOAT_T pos)
{
   Edje_Color_Class *cc = NULL;
   Edje_Internal_Aspect apref;
   int minw = 0, minh = 0, maxw = 0, maxh = 0;
   FLOAT_T sc;

   sc = ed->scale;
   if (sc == ZERO) sc = _edje_scale;
   _edje_part_recalc_single_min_max(sc, ep, desc, &minw, &minh, &maxw, &maxh);

   /* relative coords of top left & bottom right */
   _edje_part_recalc_single_rel(ed, ep, desc, rel1_to_x, rel1_to_y, rel2_to_x, rel2_to_y, params);

   /* aspect */
   apref = _edje_part_recalc_single_aspect(ed, ep, desc, params, &minw, &minh, &maxw, &maxh, pos);

   /* size step */
   _edje_part_recalc_single_step(desc, params);

   /* if we have text that wants to make the min size the text size... */
   if (ep->part->type == EDJE_PART_TYPE_TEXTBLOCK)
     _edje_part_recalc_single_textblock(sc, ed, ep, (Edje_Part_Description_Text*) chosen_desc, params, &minw, &minh, &maxw, &maxh);
   else if (ep->part->type == EDJE_PART_TYPE_TEXT)
     _edje_part_recalc_single_text(sc, ed, ep, (Edje_Part_Description_Text*) desc, (Edje_Part_Description_Text*) chosen_desc, params, &minw, &minh, &maxw, &maxh);

   if ((ep->part->type == EDJE_PART_TYPE_TABLE) &&
       (((((Edje_Part_Description_Table *)chosen_desc)->table.min.h) ||
         (((Edje_Part_Description_Table *)chosen_desc)->table.min.v))))
     {
        Evas_Coord lminw = 0, lminh = 0;

	eo_do(ep->object,
	      evas_obj_smart_need_recalculate_set(1),
	      evas_obj_smart_calculate(),
	      evas_obj_size_hint_min_get(&lminw, &lminh));
        if (((Edje_Part_Description_Table *)chosen_desc)->table.min.h)
          {
             if (lminw > minw) minw = lminw;
          }
        if (((Edje_Part_Description_Table *)chosen_desc)->table.min.v)
          {
             if (lminh > minh) minh = lminh;
          }
     }
   else if ((ep->part->type == EDJE_PART_TYPE_BOX) &&
            ((((Edje_Part_Description_Box *)chosen_desc)->box.min.h) ||
             (((Edje_Part_Description_Box *)chosen_desc)->box.min.v)))
     {
        Evas_Coord lminw = 0, lminh = 0;

	eo_do(ep->object,
	      evas_obj_smart_need_recalculate_set(1),
	      evas_obj_smart_calculate(),
	      evas_obj_size_hint_min_get(&lminw, &lminh));
        if (((Edje_Part_Description_Box *)chosen_desc)->box.min.h)
          {
             if (lminw > minw) minw = lminw;
          }
        if (((Edje_Part_Description_Box *)chosen_desc)->box.min.v)
          {
             if (lminh > minh) minh = lminh;
          }
     }
   else if ((ep->part->type == EDJE_PART_TYPE_IMAGE) &&
            (chosen_desc->min.limit || chosen_desc->max.limit))
     {
        Evas_Coord w, h;

        /* We only need pos to find the right image that would be displayed */
        /* Yes, if someone set aspect preference to SOURCE and also max,min
           to SOURCE, it will be under efficient, but who cares at the
           moment. */
        _edje_real_part_image_set(ed, ep, pos);
        evas_object_image_size_get(ep->object, &w, &h);

        if (chosen_desc->min.limit)
          {
             if (w > minw) minw = w;
             if (h > minh) minh = h;
          }
        if (chosen_desc->max.limit)
          {
             if ((maxw <= 0) || (w < maxw)) maxw = w;
             if ((maxh <= 0) || (h < maxh)) maxh = h;
          }
     }

   /* remember what our size is BEFORE we go limit it */
   params->req.x = params->x;
   params->req.y = params->y;
   params->req.w = params->w;
   params->req.h = params->h;

   /* adjust for min size */
   _edje_part_recalc_single_min(desc, params, minw, minh, apref);

   /* adjust for max size */
   _edje_part_recalc_single_max(desc, params, maxw, maxh, apref);

   /* take care of dragable part */
   if (ep->drag)
     _edje_part_recalc_single_drag(ep, confine_to, params, minw, minh, maxw, maxh);

   /* fill */
   if (ep->part->type == EDJE_PART_TYPE_IMAGE)
     _edje_part_recalc_single_fill(ep, &((Edje_Part_Description_Image *)desc)->image.fill, params);
   else if (ep->part->type == EDJE_PART_TYPE_PROXY)
     _edje_part_recalc_single_fill(ep, &((Edje_Part_Description_Proxy *)desc)->proxy.fill, params);

   if (ep->part->type != EDJE_PART_TYPE_SPACER)
     {
        /* colors */
        if ((desc->color_class) && (*desc->color_class))
          cc = _edje_color_class_find(ed, desc->color_class);

        if (cc)
          {
             params->color.r = (((int)cc->r + 1) * desc->color.r) >> 8;
             params->color.g = (((int)cc->g + 1) * desc->color.g) >> 8;
             params->color.b = (((int)cc->b + 1) * desc->color.b) >> 8;
             params->color.a = (((int)cc->a + 1) * desc->color.a) >> 8;
          }
        else
          {
             params->color.r = desc->color.r;
             params->color.g = desc->color.g;
             params->color.b = desc->color.b;
             params->color.a = desc->color.a;
          }
     }

   /* visible */
   params->visible = desc->visible;

   switch (ep->part->type)
     {
      case EDJE_PART_TYPE_IMAGE:
           {
              Edje_Part_Description_Image *img_desc = (Edje_Part_Description_Image*) desc;

              /* border */
              params->type.common.spec.image.l = img_desc->image.border.l;
              params->type.common.spec.image.r = img_desc->image.border.r;

              params->type.common.spec.image.t = img_desc->image.border.t;
              params->type.common.spec.image.b = img_desc->image.border.b;

              params->type.common.spec.image.border_scale_by = img_desc->image.border.scale_by;
              break;
           }
      case EDJE_PART_TYPE_TEXT:
      case EDJE_PART_TYPE_TEXTBLOCK:
           {
              Edje_Part_Description_Text *text_desc = (Edje_Part_Description_Text*) desc;

              /* text.align */
              params->type.text.align.x = text_desc->text.align.x;
              params->type.text.align.y = text_desc->text.align.y;
              params->type.text.elipsis = text_desc->text.elipsis;

              /* text colors */
              if (cc)
                {
                   params->type.text.color2.r = (((int)cc->r2 + 1) * text_desc->common.color2.r) >> 8;
                   params->type.text.color2.g = (((int)cc->g2 + 1) * text_desc->common.color2.g) >> 8;
                   params->type.text.color2.b = (((int)cc->b2 + 1) * text_desc->common.color2.b) >> 8;
                   params->type.text.color2.a = (((int)cc->a2 + 1) * text_desc->common.color2.a) >> 8;
                   params->type.text.color3.r = (((int)cc->r3 + 1) * text_desc->text.color3.r) >> 8;
                   params->type.text.color3.g = (((int)cc->g3 + 1) * text_desc->text.color3.g) >> 8;
                   params->type.text.color3.b = (((int)cc->b3 + 1) * text_desc->text.color3.b) >> 8;
                   params->type.text.color3.a = (((int)cc->a3 + 1) * text_desc->text.color3.a) >> 8;
                }
              else
                {
                   params->type.text.color2.r = text_desc->common.color2.r;
                   params->type.text.color2.g = text_desc->common.color2.g;
                   params->type.text.color2.b = text_desc->common.color2.b;
                   params->type.text.color2.a = text_desc->common.color2.a;
                   params->type.text.color3.r = text_desc->text.color3.r;
                   params->type.text.color3.g = text_desc->text.color3.g;
                   params->type.text.color3.b = text_desc->text.color3.b;
                   params->type.text.color3.a = text_desc->text.color3.a;
                }

              break;
           }
      case EDJE_PART_TYPE_SPACER:
      case EDJE_PART_TYPE_RECTANGLE:
      case EDJE_PART_TYPE_BOX:
      case EDJE_PART_TYPE_TABLE:
      case EDJE_PART_TYPE_SWALLOW:
      case EDJE_PART_TYPE_GROUP:
      case EDJE_PART_TYPE_PROXY:
         break;
      case EDJE_PART_TYPE_GRADIENT:
         /* FIXME: THIS ONE SHOULD NEVER BE TRIGGERED. */
         break;
      default:
         break;
     }

#ifdef HAVE_EPHYSICS
   params->physics.mass = desc->physics.mass;
   params->physics.restitution = desc->physics.restitution;
   params->physics.friction = desc->physics.friction;
   params->physics.damping.linear = desc->physics.damping.linear;
   params->physics.damping.angular = desc->physics.damping.angular;
   params->physics.sleep.linear = desc->physics.sleep.linear;
   params->physics.sleep.angular = desc->physics.sleep.angular;
   params->physics.material = desc->physics.material;
   params->physics.density = desc->physics.density;
   params->physics.hardness = desc->physics.hardness;
   params->physics.ignore_part_pos = desc->physics.ignore_part_pos;
   params->physics.light_on = desc->physics.light_on;
   params->physics.mov_freedom.lin.x = desc->physics.mov_freedom.lin.x;
   params->physics.mov_freedom.lin.y = desc->physics.mov_freedom.lin.y;
   params->physics.mov_freedom.lin.z = desc->physics.mov_freedom.lin.z;
   params->physics.mov_freedom.ang.x = desc->physics.mov_freedom.ang.x;
   params->physics.mov_freedom.ang.y = desc->physics.mov_freedom.ang.y;
   params->physics.mov_freedom.ang.z = desc->physics.mov_freedom.ang.z;
   params->physics.backcull = desc->physics.backcull;
   params->physics.z = desc->physics.z;
   params->physics.depth = desc->physics.depth;
#endif
   _edje_part_recalc_single_map(ed, ep, center, light, persp, desc, chosen_desc, params);
}

static void
_edje_table_recalc_apply(Edje *ed EINA_UNUSED,
                         Edje_Real_Part *ep,
                         Edje_Calc_Params *p3 EINA_UNUSED,
                         Edje_Part_Description_Table *chosen_desc)
{
   eo_do(ep->object,
	 evas_obj_table_homogeneous_set(chosen_desc->table.homogeneous),
	 evas_obj_table_align_set(TO_DOUBLE(chosen_desc->table.align.x), TO_DOUBLE(chosen_desc->table.align.y)),
	 evas_obj_table_padding_set(chosen_desc->table.padding.x, chosen_desc->table.padding.y));
   if (evas_object_smart_need_recalculate_get(ep->object))
     {
        eo_do(ep->object,
	      evas_obj_smart_need_recalculate_set(0),
	      evas_obj_smart_calculate());
     }
}

static void
_edje_proxy_recalc_apply(Edje *ed, Edje_Real_Part *ep, Edje_Calc_Params *p3, Edje_Part_Description_Proxy *chosen_desc, FLOAT_T pos)
{
   Edje_Real_Part *pp;
   int part_id = -1;

   if (pos >= FROM_DOUBLE(0.5))
     part_id = ((Edje_Part_Description_Proxy*) ep->param2->description)->proxy.id;
   else
     part_id = chosen_desc->proxy.id;

   if ((p3->type.common.fill.w == 0) || (p3->type.common.fill.h == 0) ||
       (part_id < 0))
     {
        evas_object_image_source_set(ep->object, NULL);
        return;
     }
   pp = ed->table_parts[part_id % ed->table_parts_size];

   if (pp->nested_smart)  /* using nested_smart for nested parts */
     {
        evas_object_image_source_set(ep->object, pp->nested_smart);
     }
   else
     {
        switch (pp->part->type)
          {
           case EDJE_PART_TYPE_IMAGE:
           case EDJE_PART_TYPE_TEXT:
           case EDJE_PART_TYPE_TEXTBLOCK:
           case EDJE_PART_TYPE_RECTANGLE:
           case EDJE_PART_TYPE_BOX:
           case EDJE_PART_TYPE_TABLE:
           case EDJE_PART_TYPE_PROXY:
              evas_object_image_source_set(ep->object, pp->object);
              break;
           case EDJE_PART_TYPE_GRADIENT:
              /* FIXME: THIS ONE SHOULD NEVER BE TRIGGERED. */
              break;
           case EDJE_PART_TYPE_GROUP:
           case EDJE_PART_TYPE_SWALLOW:
           case EDJE_PART_TYPE_EXTERNAL:
             if ((pp->type == EDJE_RP_TYPE_SWALLOW) &&
                 (pp->typedata.swallow))
               {
                  evas_object_image_source_set(ep->object, pp->typedata.swallow->swallowed_object);
               }
              break;
           case EDJE_PART_TYPE_SPACER:
              /* FIXME: detect that at compile time and prevent it */
              break;
          }
     }

   eo_do(ep->object,
	 evas_obj_image_fill_set(p3->type.common.fill.x, p3->type.common.fill.y,
				 p3->type.common.fill.w, p3->type.common.fill.h),
	 evas_obj_image_smooth_scale_set(p3->smooth));
}

static void
_edje_image_recalc_apply(Edje *ed, Edje_Real_Part *ep, Edje_Calc_Params *p3, Edje_Part_Description_Image *chosen_desc, FLOAT_T pos)
{
   FLOAT_T sc;

   sc = ed->scale;
   if (sc == 0.0) sc = _edje_scale;
   eo_do(ep->object,
	 evas_obj_image_fill_set(p3->type.common.fill.x, p3->type.common.fill.y,
				 p3->type.common.fill.w, p3->type.common.fill.h),
	 evas_obj_image_smooth_scale_set(p3->smooth));
   if (chosen_desc->image.border.scale)
     {
        if (p3->type.common.spec.image.border_scale_by > FROM_DOUBLE(0.0))
          {
             FLOAT_T sc2 = MUL(sc, p3->type.common.spec.image.border_scale_by);
             evas_object_image_border_scale_set(ep->object, TO_DOUBLE(sc2));
          }
        else
           evas_object_image_border_scale_set(ep->object, TO_DOUBLE(sc));
     }
   else
     {
        if (p3->type.common.spec.image.border_scale_by > FROM_DOUBLE(0.0))
           evas_object_image_border_scale_set
           (ep->object, TO_DOUBLE(p3->type.common.spec.image.border_scale_by));
        else
          evas_object_image_border_scale_set(ep->object, 1.0);
     }
   evas_object_image_border_set(ep->object, p3->type.common.spec.image.l, p3->type.common.spec.image.r,
                                p3->type.common.spec.image.t, p3->type.common.spec.image.b);
   if (chosen_desc->image.border.no_fill == 0)
     evas_object_image_border_center_fill_set(ep->object, EVAS_BORDER_FILL_DEFAULT);
   else if (chosen_desc->image.border.no_fill == 1)
     evas_object_image_border_center_fill_set(ep->object, EVAS_BORDER_FILL_NONE);
   else if (chosen_desc->image.border.no_fill == 2)
     evas_object_image_border_center_fill_set(ep->object, EVAS_BORDER_FILL_SOLID);

   _edje_real_part_image_set(ed, ep, pos);
}

static Edje_Real_Part *
_edje_real_part_state_get(Edje *ed, Edje_Real_Part *ep, int flags, int id, int *state)
{
   Edje_Real_Part *result = NULL;

   if (id >= 0 && id != ep->part->id)
     {
        result = ed->table_parts[id % ed->table_parts_size];
        if (result)
          {
             if (!result->calculated) _edje_part_recalc(ed, result, flags, NULL);
#ifdef EDJE_CALC_CACHE
             if (state) *state = result->state;
#else
             (void) state;
#endif
          }
     }
   return result;
}

#ifdef HAVE_EPHYSICS
static Eina_Bool
_edje_physics_world_geometry_check(EPhysics_World *world)
{
   Evas_Coord w, h, d;
   ephysics_world_render_geometry_get(world, NULL, NULL, NULL, &w, &h, &d);
   return (w && h && d);
}

static void
_edje_physics_body_props_update(Edje_Real_Part *ep, Edje_Calc_Params *pf, Eina_Bool pos_update)
{
   ephysics_body_linear_movement_enable_set(ep->body,
                                            pf->physics.mov_freedom.lin.x,
                                            pf->physics.mov_freedom.lin.y,
                                            pf->physics.mov_freedom.lin.z);
   ephysics_body_angular_movement_enable_set(ep->body,
                                             pf->physics.mov_freedom.ang.x,
                                             pf->physics.mov_freedom.ang.y,
                                             pf->physics.mov_freedom.ang.z);

   /* Boundaries geometry and mass shouldn't be changed */
   if (ep->part->physics_body < EDJE_PART_PHYSICS_BODY_BOUNDARY_TOP)
     {
        Evas_Coord x, y, z, w, h, d;

        if (pos_update)
          {
             ephysics_body_move(ep->body, ep->edje->x + pf->x,
                                ep->edje->y + pf->y, pf->physics.z);
             ep->x = pf->x;
             ep->y = pf->y;
             ep->w = pf->w;
             ep->h = pf->h;
          }

        ephysics_body_geometry_get(ep->body, &x, &y, &z, &w, &h, &d);
        if ((d) && (d != pf->physics.depth))
          ephysics_body_resize(ep->body, w, h, pf->physics.depth);
        if (z != pf->physics.z)
          ephysics_body_move(ep->body, x, y, pf->physics.z);

        ephysics_body_material_set(ep->body, pf->physics.material);
        if (!pf->physics.material)
          {
             if (pf->physics.density)
               ephysics_body_density_set(ep->body, pf->physics.density);
             else
               ephysics_body_mass_set(ep->body, pf->physics.mass);
          }

        if ((ep->part->physics_body == EDJE_PART_PHYSICS_BODY_SOFT_BOX) ||
            (ep->part->physics_body == EDJE_PART_PHYSICS_BODY_SOFT_SPHERE) ||
            (ep->part->physics_body == EDJE_PART_PHYSICS_BODY_SOFT_CYLINDER) ||
            (ep->part->physics_body == EDJE_PART_PHYSICS_BODY_CLOTH))
          ephysics_body_soft_body_hardness_set(ep->body,
                                               pf->physics.hardness * 100);
     }

   if (!pf->physics.material)
     {
        ephysics_body_restitution_set(ep->body, pf->physics.restitution);
        ephysics_body_friction_set(ep->body, pf->physics.friction);
     }

   ephysics_body_damping_set(ep->body, pf->physics.damping.linear,
                             pf->physics.damping.angular);
   ephysics_body_sleeping_threshold_set(ep->body, pf->physics.sleep.linear,
                                        pf->physics.sleep.angular);
   ephysics_body_light_set(ep->body, pf->physics.light_on);
   ephysics_body_back_face_culling_set(ep->body, pf->physics.backcull);
}

static void
_edje_physics_body_update_cb(void *data, EPhysics_Body *body, void *event_info EINA_UNUSED)
{
   Edje_Real_Part *rp = data;
   ephysics_body_geometry_get(body, &(rp->x), &(rp->y), NULL,
                              &(rp->w), &(rp->h), NULL);
   ephysics_body_evas_object_update(body);
   rp->edje->dirty = EINA_TRUE;
}

static void
_edje_physics_body_add(Edje_Real_Part *rp, EPhysics_World *world)
{
   Eina_Bool resize = EINA_TRUE;
   Edje_Physics_Face *pface;
   Eina_List *l;

   switch (rp->part->physics_body)
     {
      case EDJE_PART_PHYSICS_BODY_RIGID_BOX:
         rp->body = ephysics_body_box_add(world);
         break;
      case EDJE_PART_PHYSICS_BODY_RIGID_SPHERE:
         rp->body = ephysics_body_sphere_add(world);
         break;
      case EDJE_PART_PHYSICS_BODY_RIGID_CYLINDER:
         rp->body = ephysics_body_cylinder_add(world);
         break;
      case EDJE_PART_PHYSICS_BODY_SOFT_BOX:
         rp->body = ephysics_body_soft_box_add(world);
         break;
      case EDJE_PART_PHYSICS_BODY_SOFT_SPHERE:
         rp->body = ephysics_body_soft_sphere_add(world, 0);
         break;
      case EDJE_PART_PHYSICS_BODY_SOFT_CYLINDER:
         rp->body = ephysics_body_soft_cylinder_add(world);
         break;
      case EDJE_PART_PHYSICS_BODY_CLOTH:
         rp->body = ephysics_body_cloth_add(world, 0, 0);
         break;
      case EDJE_PART_PHYSICS_BODY_BOUNDARY_TOP:
         rp->body = ephysics_body_top_boundary_add(world);
         resize = EINA_FALSE;
         break;
      case EDJE_PART_PHYSICS_BODY_BOUNDARY_BOTTOM:
         rp->body = ephysics_body_bottom_boundary_add(world);
         resize = EINA_FALSE;
         break;
      case EDJE_PART_PHYSICS_BODY_BOUNDARY_RIGHT:
         rp->body = ephysics_body_right_boundary_add(world);
         resize = EINA_FALSE;
         break;
      case EDJE_PART_PHYSICS_BODY_BOUNDARY_LEFT:
         rp->body = ephysics_body_left_boundary_add(world);
         resize = EINA_FALSE;
         break;
      case EDJE_PART_PHYSICS_BODY_BOUNDARY_FRONT:
         rp->body = ephysics_body_front_boundary_add(world);
         resize = EINA_FALSE;
         break;
      case EDJE_PART_PHYSICS_BODY_BOUNDARY_BACK:
         rp->body = ephysics_body_back_boundary_add(world);
         resize = EINA_FALSE;
         break;
      default:
         ERR("Invalid body: %i", rp->part->physics_body);
         return;
     }

   EINA_LIST_FOREACH(rp->part->default_desc->physics.faces, l, pface)
     {
        Evas_Object *edje_obj;
        Evas *evas;

        if (!pface->source) continue;

        evas = evas_object_evas_get(rp->object);
        edje_obj = edje_object_add(evas);
        if (!edje_obj) continue;

        edje_object_file_set(edje_obj, rp->edje->path, pface->source);
        evas_object_resize(edje_obj, 1, 1);
        ephysics_body_face_evas_object_set(rp->body, pface->type,
                                           edje_obj, EINA_FALSE);
        rp->body_faces = eina_list_append(rp->body_faces, edje_obj);
     }

   ephysics_body_evas_object_set(rp->body, rp->object, resize);
   ephysics_body_event_callback_add(rp->body, EPHYSICS_CALLBACK_BODY_UPDATE,
                                    _edje_physics_body_update_cb, rp);
}
#endif

void
_edje_part_recalc(Edje *ed, Edje_Real_Part *ep, int flags, Edje_Calc_Params *state)
{
#ifdef EDJE_CALC_CACHE
   Eina_Bool proxy_invalidate = EINA_FALSE;
   int state1 = -1;
   int state2 = -1;
   int statec = -1;
#else
   Edje_Calc_Params lp1, lp2;
#endif
   int statec1 = -1;
   int statec2 = -1;
   int statel1 = -1;
   int statel2 = -1;
   int statep1 = -1;
   int statep2 = -1;
   Edje_Real_Part *center[2] = { NULL, NULL };
   Edje_Real_Part *light[2] = { NULL, NULL };
   Edje_Real_Part *persp[2] = { NULL, NULL };
   Edje_Calc_Params *p1, *pf;
   Edje_Part_Description_Common *chosen_desc;
   Edje_Real_Part *confine_to = NULL;
   FLOAT_T pos = ZERO, pos2;
   Edje_Calc_Params lp3;

   /* GRADIENT ARE GONE, WE MUST IGNORE IT FROM OLD FILE. */
   if (ep->part->type == EDJE_PART_TYPE_GRADIENT)
     {
        ERR("GRADIENT spotted during recalc ! That should never happen ! Send your edje file to devel ml.");
        return;
     }

   if ((ep->calculated & FLAG_XY) == FLAG_XY && !state)
     {
        return;
     }
   if (ep->calculating & flags)
     {
#if 1
        const char *axes = "NONE", *faxes = "NONE";

        if ((ep->calculating & FLAG_X) &&
            (ep->calculating & FLAG_Y))
          axes = "XY";
        else if ((ep->calculating & FLAG_X))
          axes = "X";
        else if ((ep->calculating & FLAG_Y))
          axes = "Y";

        if ((flags & FLAG_X) &&
            (flags & FLAG_Y))
          faxes = "XY";
        else if ((flags & FLAG_X))
          faxes = "X";
        else if ((flags & FLAG_Y))
          faxes = "Y";
        ERR("Circular dependency when calculating part \"%s\". "
            "Already calculating %s [%02x] axes. "
            "Need to calculate %s [%02x] axes",
            ep->part->name,
            axes, ep->calculating,
            faxes, flags);
#endif
        return;
     }

   if (ep->part->scale &&
       ep->part->type == EDJE_PART_TYPE_GROUP &&
       ((ep->type == EDJE_RP_TYPE_SWALLOW) &&
           (ep->typedata.swallow)) &&
       ep->typedata.swallow->swallowed_object)
     {
        edje_object_scale_set(ep->typedata.swallow->swallowed_object, TO_DOUBLE(ed->scale));

        if (ep->description_pos > FROM_DOUBLE(0.5) && ep->param2)
          {
             edje_object_update_hints_set(ep->typedata.swallow->swallowed_object, ep->param2->description->min.limit);
          }
        else
          {
             edje_object_update_hints_set(ep->typedata.swallow->swallowed_object, ep->param1.description->min.limit);
          }
        if (edje_object_update_hints_get(ep->typedata.swallow->swallowed_object))
          {
             Edje *ted;

             ted = _edje_fetch(ep->typedata.swallow->swallowed_object);
             _edje_recalc_do(ted);
          }
     }

#ifdef EDJE_CALC_CACHE
   if (ep->state == ed->state && !state)
     return ;
#endif

   if (flags & FLAG_X)
     {
        ep->calculating |= flags & FLAG_X;
        if (ep->param1.rel1_to_x)
          {
             _edje_part_recalc(ed, ep->param1.rel1_to_x, FLAG_X, NULL);
#ifdef EDJE_CALC_CACHE
             state1 = ep->param1.rel1_to_x->state;
#endif
          }
        if (ep->param1.rel2_to_x)
          {
             _edje_part_recalc(ed, ep->param1.rel2_to_x, FLAG_X, NULL);
#ifdef EDJE_CALC_CACHE
             if (state1 < ep->param1.rel2_to_x->state)
               state1 = ep->param1.rel2_to_x->state;
#endif
          }
        if (ep->param2)
          {
             if (ep->param2->rel1_to_x)
               {
                  _edje_part_recalc(ed, ep->param2->rel1_to_x, FLAG_X, NULL);
#ifdef EDJE_CALC_CACHE
                  state2 = ep->param2->rel1_to_x->state;
#endif
               }
             if (ep->param2->rel2_to_x)
               {
                  _edje_part_recalc(ed, ep->param2->rel2_to_x, FLAG_X, NULL);
#ifdef EDJE_CALC_CACHE
                  if (state2 < ep->param2->rel2_to_x->state)
                    state2 = ep->param2->rel2_to_x->state;
#endif
               }
          }
     }
   if (flags & FLAG_Y)
     {
        ep->calculating |= flags & FLAG_Y;
        if (ep->param1.rel1_to_y)
          {
             _edje_part_recalc(ed, ep->param1.rel1_to_y, FLAG_Y, NULL);
#ifdef EDJE_CALC_CACHE
             if (state1 < ep->param1.rel1_to_y->state)
               state1 = ep->param1.rel1_to_y->state;
#endif
          }
        if (ep->param1.rel2_to_y)
          {
             _edje_part_recalc(ed, ep->param1.rel2_to_y, FLAG_Y, NULL);
#ifdef EDJE_CALC_CACHE
             if (state1 < ep->param1.rel2_to_y->state)
               state1 = ep->param1.rel2_to_y->state;
#endif
          }
        if (ep->param2)
          {
             if (ep->param2->rel1_to_y)
               {
                  _edje_part_recalc(ed, ep->param2->rel1_to_y, FLAG_Y, NULL);
#ifdef EDJE_CALC_CACHE
                  if (state2 < ep->param2->rel1_to_y->state)
                    state2 = ep->param2->rel1_to_y->state;
#endif
               }
             if (ep->param2->rel2_to_y)
               {
                  _edje_part_recalc(ed, ep->param2->rel2_to_y, FLAG_Y, NULL);
#ifdef EDJE_CALC_CACHE
                  if (state2 < ep->param2->rel2_to_y->state)
                    state2 = ep->param2->rel2_to_y->state;
#endif
               }
          }
     }
   if (ep->drag && ep->drag->confine_to)
     {
        confine_to = ep->drag->confine_to;
        _edje_part_recalc(ed, confine_to, flags, NULL);
#ifdef EDJE_CALC_CACHE
        statec = confine_to->state;
#endif
     }
   //   if (ep->text.source)       _edje_part_recalc(ed, ep->text.source, flags);
   //   if (ep->text.text_source)  _edje_part_recalc(ed, ep->text.text_source, flags);

   /* actually calculate now */
   chosen_desc = ep->chosen_description;
   if (!chosen_desc)
     {
        ep->calculating = FLAG_NONE;
        ep->calculated |= flags;
        return;
     }

   pos = ep->description_pos;

   if (ep->part->type == EDJE_PART_TYPE_PROXY)
     {
        int part_id = -1;

        if (pos >= FROM_DOUBLE(0.5))
          part_id = ((Edje_Part_Description_Proxy*) ep->param2->description)->proxy.id;
        else
          part_id = ((Edje_Part_Description_Proxy*) chosen_desc)->proxy.id;

#ifdef EDJE_CALC_CACHE
        Edje_Real_Part *pp;
        pp = _edje_real_part_state_get(ed, ep, flags, part_id, NULL);
        if (pp && pp->invalidate) proxy_invalidate = EINA_TRUE;
#else
        _edje_real_part_state_get(ed, ep, flags, part_id, NULL);
#endif
     }

   /* Recalc if needed the map center && light source */
   if (ep->param1.description->map.on)
     {
        center[0] = _edje_real_part_state_get(ed, ep, flags, ep->param1.description->map.rot.id_center, &statec1);
        light[0] = _edje_real_part_state_get(ed, ep, flags, ep->param1.description->map.id_light, &statel1);

        if (chosen_desc->map.persp_on)
          {
             persp[0] = _edje_real_part_state_get(ed, ep, flags, ep->param1.description->map.id_persp, &statep1);
          }
     }

   if (ep->param2 && ep->param2->description->map.on)
     {
        center[1] = _edje_real_part_state_get(ed, ep, flags, ep->param2->description->map.rot.id_center, &statec2);
        light[1] = _edje_real_part_state_get(ed, ep, flags, ep->param2->description->map.id_light, &statel2);

        if (chosen_desc->map.persp_on)
          {
             persp[1] = _edje_real_part_state_get(ed, ep, flags, ep->param2->description->map.id_persp, &statep2);
          }
     }

#ifndef EDJE_CALC_CACHE
   p1 = &lp1;
#else
   p1 = &ep->param1.p;
#endif

   if (ep->param1.description)
     {
#ifdef EDJE_CALC_CACHE
        if (ed->all_part_change ||
            ep->invalidate ||
            state1 >= ep->param1.state ||
            statec >= ep->param1.state ||
            statec1 >= ep->param1.state ||
            statel1 >= ep->param1.state ||
            statep1 >= ep->param1.state ||
            proxy_invalidate ||
            state ||
            ((ep->part->type == EDJE_PART_TYPE_TEXT || ep->part->type == EDJE_PART_TYPE_TEXTBLOCK) && ed->text_part_change))
#endif
          {
             _edje_part_recalc_single(ed, ep, ep->param1.description, chosen_desc, center[0], light[0], persp[0],
                                      ep->param1.rel1_to_x, ep->param1.rel1_to_y, ep->param1.rel2_to_x, ep->param1.rel2_to_y,
                                      confine_to,
                                      p1, pos);
#ifdef EDJE_CALC_CACHE
             if (flags == FLAG_XY)
               ep->param1.state = ed->state;
#endif
          }
     }
   if (ep->param2)
     {
        int beginning_pos, part_type;
        Edje_Calc_Params *p2, *p3;

        if (ep->current)
          {
             /* FIXME: except for text, we don't need in that case to recalc p1 at all*/
             memcpy(p1, ep->current, sizeof (Edje_Calc_Params));
             p1->x += ed->x;
             p1->y += ed->y;
             p1->map.center.x += ed->x;
             p1->map.center.y += ed->y;
             p1->map.light.x += ed->x;
             p1->map.light.y += ed->y;
             p1->map.persp.x += ed->x;
             p1->map.persp.y += ed->y;
          }

        p3 = &lp3;

#ifndef EDJE_CALC_CACHE
        p2 = &lp2;
#else
        p2 = &ep->param2->p;

        if (ed->all_part_change ||
            ep->invalidate ||
            state2 >= ep->param2->state ||
            statec >= ep->param2->state ||
            statec2 >= ep->param2->state ||
            statel2 >= ep->param2->state ||
            statep2 >= ep->param2->state ||
            proxy_invalidate ||
            state ||
            ((ep->part->type == EDJE_PART_TYPE_TEXT || ep->part->type == EDJE_PART_TYPE_TEXTBLOCK) && ed->text_part_change))
#endif
          {
             _edje_part_recalc_single(ed, ep, ep->param2->description,
                                      chosen_desc,
                                      center[1], light[1], persp[1],
                                      ep->param2->rel1_to_x,
                                      ep->param2->rel1_to_y,
                                      ep->param2->rel2_to_x,
                                      ep->param2->rel2_to_y,
                                      confine_to,
                                      p2, pos);
#ifdef EDJE_CALC_CACHE
             if (flags == FLAG_XY)
               ep->param2->state = ed->state;
#endif
          }

        pos2 = pos;
        if (pos2 < ZERO) pos2 = ZERO;
        else if (pos2 > FROM_INT(1)) pos2 = FROM_INT(1);
        beginning_pos = (pos < FROM_DOUBLE(0.5));
        part_type = ep->part->type;

        /* visible is special */
        if ((p1->visible) && (!p2->visible))
          p3->visible = (pos != FROM_INT(1));
        else if ((!p1->visible) && (p2->visible))
          p3->visible = (pos != ZERO);
        else
          p3->visible = p1->visible;

        p3->smooth = (beginning_pos) ? p1->smooth : p2->smooth;

        /* FIXME: do x and y separately base on flag */
#define FINTP(_x1, _x2, _p)                     \
        (((_x1) == (_x2))                       \
         ? FROM_INT((_x1))                      \
         : ADD(FROM_INT(_x1),                   \
               SCALE((_p), (_x2) - (_x1))))

#define FFP(_x1, _x2, _p)                       \
        (((_x1) == (_x2))                       \
         ? (_x1)                                \
         : ADD(_x1, MUL(_p, SUB(_x2, _x1))));

#define INTP(_x1, _x2, _p) TO_INT(FINTP(_x1, _x2, _p))

        p3->x = INTP(p1->x, p2->x, pos);
        p3->y = INTP(p1->y, p2->y, pos);
        p3->w = INTP(p1->w, p2->w, pos);
        p3->h = INTP(p1->h, p2->h, pos);

        p3->req.x = INTP(p1->req.x, p2->req.x, pos);
        p3->req.y = INTP(p1->req.y, p2->req.y, pos);
        p3->req.w = INTP(p1->req.w, p2->req.w, pos);
        p3->req.h = INTP(p1->req.h, p2->req.h, pos);

        if (ep->part->dragable.x)
          {
             p3->req_drag.x = INTP(p1->req_drag.x, p2->req_drag.x, pos);
             p3->req_drag.w = INTP(p1->req_drag.w, p2->req_drag.w, pos);
          }
        if (ep->part->dragable.y)
          {
             p3->req_drag.y = INTP(p1->req_drag.y, p2->req_drag.y, pos);
             p3->req_drag.h = INTP(p1->req_drag.h, p2->req_drag.h, pos);
          }

        p3->color.r = INTP(p1->color.r, p2->color.r, pos2);
        p3->color.g = INTP(p1->color.g, p2->color.g, pos2);
        p3->color.b = INTP(p1->color.b, p2->color.b, pos2);
        p3->color.a = INTP(p1->color.a, p2->color.a, pos2);

#ifdef HAVE_EPHYSICS
        p3->physics.mass = TO_DOUBLE(FINTP(p1->physics.mass, p2->physics.mass,
                                         pos));
        p3->physics.restitution = TO_DOUBLE(FINTP(p1->physics.restitution,
                                                  p2->physics.restitution,
                                                  pos));
        p3->physics.friction = TO_DOUBLE(FINTP(p1->physics.friction,
                                               p2->physics.friction, pos));
        p3->physics.density = TO_DOUBLE(FINTP(p1->physics.density,
                                              p2->physics.density, pos));
        p3->physics.hardness = TO_DOUBLE(FINTP(p1->physics.hardness,
                                               p2->physics.hardness, pos));

        p3->physics.damping.linear = TO_DOUBLE(FINTP(
              p1->physics.damping.linear, p2->physics.damping.linear, pos));
        p3->physics.damping.angular = TO_DOUBLE(FINTP(
              p1->physics.damping.angular, p2->physics.damping.angular, pos));

        p3->physics.sleep.linear = TO_DOUBLE(FINTP(
              p1->physics.sleep.linear, p2->physics.sleep.linear, pos));
        p3->physics.sleep.angular = TO_DOUBLE(FINTP(
              p1->physics.sleep.angular, p2->physics.sleep.angular, pos));

        p3->physics.z = INTP(p1->physics.z, p2->physics.z, pos);
        p3->physics.depth = INTP(p1->physics.depth, p2->physics.depth, pos);

        if ((p1->physics.ignore_part_pos) && (p2->physics.ignore_part_pos))
          p3->physics.ignore_part_pos = 1;
        else
          p3->physics.ignore_part_pos = 0;

        if ((p1->physics.material) && (p2->physics.material))
          p3->physics.material = p1->physics.material;
        else
          p3->physics.material = EPHYSICS_BODY_MATERIAL_CUSTOM;

        p3->physics.light_on = p1->physics.light_on || p2->physics.light_on;
        p3->physics.backcull = p1->physics.backcull || p2->physics.backcull;

        p3->physics.mov_freedom.lin.x = p1->physics.mov_freedom.lin.x ||
           p2->physics.mov_freedom.lin.x;
        p3->physics.mov_freedom.lin.y = p1->physics.mov_freedom.lin.y ||
           p2->physics.mov_freedom.lin.y;
        p3->physics.mov_freedom.lin.z = p1->physics.mov_freedom.lin.z ||
           p2->physics.mov_freedom.lin.z;
        p3->physics.mov_freedom.ang.x = p1->physics.mov_freedom.ang.x ||
           p2->physics.mov_freedom.ang.x;
        p3->physics.mov_freedom.ang.y = p1->physics.mov_freedom.ang.y ||
           p2->physics.mov_freedom.ang.y;
        p3->physics.mov_freedom.ang.z = p1->physics.mov_freedom.ang.z ||
           p2->physics.mov_freedom.ang.z;
#endif

        switch (part_type)
          {
           case EDJE_PART_TYPE_IMAGE:
              p3->type.common.spec.image.l = INTP(p1->type.common.spec.image.l, p2->type.common.spec.image.l, pos);
              p3->type.common.spec.image.r = INTP(p1->type.common.spec.image.r, p2->type.common.spec.image.r, pos);
              p3->type.common.spec.image.t = INTP(p1->type.common.spec.image.t, p2->type.common.spec.image.t, pos);
              p3->type.common.spec.image.b = INTP(p1->type.common.spec.image.b, p2->type.common.spec.image.b, pos);
              p3->type.common.spec.image.border_scale_by = INTP(p1->type.common.spec.image.border_scale_by, p2->type.common.spec.image.border_scale_by, pos);
           case EDJE_PART_TYPE_PROXY:
              p3->type.common.fill.x = INTP(p1->type.common.fill.x, p2->type.common.fill.x, pos);
              p3->type.common.fill.y = INTP(p1->type.common.fill.y, p2->type.common.fill.y, pos);
              p3->type.common.fill.w = INTP(p1->type.common.fill.w, p2->type.common.fill.w, pos);
              p3->type.common.fill.h = INTP(p1->type.common.fill.h, p2->type.common.fill.h, pos);
              break;
           case EDJE_PART_TYPE_TEXT:
              p3->type.text.size = INTP(p1->type.text.size, p2->type.text.size, pos);
           case EDJE_PART_TYPE_TEXTBLOCK:
              p3->type.text.color2.r = INTP(p1->type.text.color2.r, p2->type.text.color2.r, pos2);
              p3->type.text.color2.g = INTP(p1->type.text.color2.g, p2->type.text.color2.g, pos2);
              p3->type.text.color2.b = INTP(p1->type.text.color2.b, p2->type.text.color2.b, pos2);
              p3->type.text.color2.a = INTP(p1->type.text.color2.a, p2->type.text.color2.a, pos2);

              p3->type.text.color3.r = INTP(p1->type.text.color3.r, p2->type.text.color3.r, pos2);
              p3->type.text.color3.g = INTP(p1->type.text.color3.g, p2->type.text.color3.g, pos2);
              p3->type.text.color3.b = INTP(p1->type.text.color3.b, p2->type.text.color3.b, pos2);
              p3->type.text.color3.a = INTP(p1->type.text.color3.a, p2->type.text.color3.a, pos2);

              p3->type.text.align.x = FFP(p1->type.text.align.x, p2->type.text.align.x, pos);
              p3->type.text.align.y = FFP(p1->type.text.align.y, p2->type.text.align.y, pos);
              p3->type.text.elipsis = TO_DOUBLE(FINTP(p1->type.text.elipsis, p2->type.text.elipsis, pos2));
              break;
          }

        p3->mapped = p1->mapped;
        p3->persp_on = p3->mapped ? p1->persp_on | p2->persp_on : 0;
        p3->lighted = p3->mapped ? p1->lighted | p2->lighted : 0;
        if (p1->mapped)
          {
             p3->map.center.x = INTP(p1->map.center.x, p2->map.center.x, pos);
             p3->map.center.y = INTP(p1->map.center.y, p2->map.center.y, pos);
             p3->map.center.z = INTP(p1->map.center.z, p2->map.center.z, pos);
             p3->map.rotation.x = FFP(p1->map.rotation.x, p2->map.rotation.x, pos);
             p3->map.rotation.y = FFP(p1->map.rotation.y, p2->map.rotation.y, pos);
             p3->map.rotation.z = FFP(p1->map.rotation.z, p2->map.rotation.z, pos);

#define MIX(P1, P2, P3, pos, info)              \
             P3->info = P1->info + TO_INT(SCALE(pos, P2->info - P1->info));

             if (p1->lighted && p2->lighted)
               {
                  MIX(p1, p2, p3, pos, map.light.x);
                  MIX(p1, p2, p3, pos, map.light.y);
                  MIX(p1, p2, p3, pos, map.light.z);
                  MIX(p1, p2, p3, pos, map.light.r);
                  MIX(p1, p2, p3, pos, map.light.g);
                  MIX(p1, p2, p3, pos, map.light.b);
                  MIX(p1, p2, p3, pos, map.light.ar);
                  MIX(p1, p2, p3, pos, map.light.ag);
                  MIX(p1, p2, p3, pos, map.light.ab);
               }
             else if (p1->lighted)
               {
                  memcpy(&p3->map.light, &p1->map.light, sizeof (p1->map.light));
               }
             else if (p2->lighted)
               {
                  memcpy(&p3->map.light, &p2->map.light, sizeof (p2->map.light));
               }

             if (p1->persp_on && p2->persp_on)
               {
                  MIX(p1, p2, p3, pos, map.persp.x);
                  MIX(p1, p2, p3, pos, map.persp.y);
                  MIX(p1, p2, p3, pos, map.persp.z);
                  MIX(p1, p2, p3, pos, map.persp.focal);
               }
             else if (p1->persp_on)
               {
                  memcpy(&p3->map.persp, &p1->map.persp, sizeof (p1->map.persp));
               }
             else if (p2->persp_on)
               {
                  memcpy(&p3->map.persp, &p2->map.persp, sizeof (p2->map.persp));
               }
          }

        pf = p3;
     }
   else
     {
        pf = p1;
     }

   if (!pf->persp_on && chosen_desc->map.persp_on)
     {
        if (ed->persp)
          {
             pf->map.persp.x = ed->persp->px;
             pf->map.persp.y = ed->persp->py;
             pf->map.persp.z = ed->persp->z0;
             pf->map.persp.focal = ed->persp->foc;
          }
        else
          {
             const Edje_Perspective *ps;

             // fixme: a tad inefficient as this is a has lookup
             ps = edje_object_perspective_get(ed->obj);
             if (!ps)
               ps = edje_evas_global_perspective_get(evas_object_evas_get(ed->obj));
             if (ps)
               {
                  pf->map.persp.x = ps->px;
                  pf->map.persp.y = ps->py;
                  pf->map.persp.z = ps->z0;
                  pf->map.persp.focal = ps->foc;
               }
             else
               {
                  pf->map.persp.x = ed->x + (ed->w / 2);
                  pf->map.persp.y = ed->y + (ed->h / 2);
                  pf->map.persp.z = 0;
                  pf->map.persp.focal = 1000;
               }
          }
     }

   if (state)
     {
        memcpy(state, pf, sizeof (Edje_Calc_Params));
     }

   ep->req = pf->req;

   if (ep->drag && ep->drag->need_reset)
     {
        FLOAT_T dx, dy;

        dx = ZERO;
        dy = ZERO;
        _edje_part_dragable_calc(ed, ep, &dx, &dy);
        ep->drag->x = dx;
        ep->drag->y = dy;
        ep->drag->tmp.x = 0;
        ep->drag->tmp.y = 0;
        ep->drag->need_reset = 0;
     }
   if (!ed->calc_only)
     {
        Evas_Object *mo;

        /* Common move, resize and color_set for all part. */
        switch (ep->part->type)
          {
           case EDJE_PART_TYPE_IMAGE:
                {
                   Edje_Part_Description_Image *img_desc = (Edje_Part_Description_Image*) chosen_desc;

                   evas_object_image_scale_hint_set(ep->object,
                                                    img_desc->image.scale_hint);
                }
           case EDJE_PART_TYPE_PROXY:
           case EDJE_PART_TYPE_RECTANGLE:
           case EDJE_PART_TYPE_TEXTBLOCK:
           case EDJE_PART_TYPE_BOX:
           case EDJE_PART_TYPE_TABLE:
              evas_object_color_set(ep->object,
                                    (pf->color.r * pf->color.a) / 255,
                                    (pf->color.g * pf->color.a) / 255,
                                    (pf->color.b * pf->color.a) / 255,
                                    pf->color.a);

#ifdef HAVE_EPHYSICS
/* body attributes should be updated for invisible objects */
              if (!ep->part->physics_body)
                {
                   if (!pf->visible)
                     {
                        evas_object_hide(ep->object);
                        break;
                     }
                   evas_object_show(ep->object);
                }
              else if (!pf->visible)
                {
                   Evas_Object *face_obj;
                   Eina_List *l;

                   EINA_LIST_FOREACH(ep->body_faces, l, face_obj)
                      evas_object_hide(face_obj);
                   evas_object_hide(ep->object);
                }
#else
              if (!pf->visible)
                {
                   evas_object_hide(ep->object);
                   break;
                }
              evas_object_show(ep->object);
#endif
              /* move and resize are needed for all previous object => no break here. */
           case EDJE_PART_TYPE_SWALLOW:
           case EDJE_PART_TYPE_GROUP:
           case EDJE_PART_TYPE_EXTERNAL:
              /* visibility and color have no meaning on SWALLOW and GROUP part. */
#ifdef HAVE_EPHYSICS
              eo_do(ep->object,
                    evas_obj_size_set(pf->w, pf->h));
              if ((ep->part->physics_body) && (!ep->body))
                {
                   if (_edje_physics_world_geometry_check(ep->edje->world))
                     {
                        _edje_physics_body_add(ep, ep->edje->world);
                        _edje_physics_body_props_update(ep, pf, EINA_TRUE);
                     }
                }
              else if (ep->body)
                {
                   if (((ep->prev_description) &&
                        (chosen_desc != ep->prev_description)) ||
                       (pf != p1))
                     _edje_physics_body_props_update(
                        ep, pf, !pf->physics.ignore_part_pos);
                }
              else
                eo_do(ep->object,
                      evas_obj_position_set(ed->x + pf->x, ed->y + pf->y));
#else
	      eo_do(ep->object,
                    evas_obj_position_set(ed->x + pf->x, ed->y + pf->y),
		    evas_obj_size_set(pf->w, pf->h));
#endif

              if (ep->nested_smart)
                {  /* Move, Resize all nested parts */
                   /* Not really needed but will improve the bounding box evaluation done by Evas */
		   eo_do(ep->nested_smart,
			 evas_obj_position_set(ed->x + pf->x, ed->y + pf->y),
			 evas_obj_size_set(pf->w, pf->h));
                }
              if (ep->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
                _edje_entry_real_part_configure(ep);
              break;
           case EDJE_PART_TYPE_TEXT:
              /* This is correctly handle in _edje_text_recalc_apply at the moment. */
              break;
           case EDJE_PART_TYPE_GRADIENT:
              /* FIXME: definitivly remove this code when we switch to new format. */
              abort();
              break;
           case EDJE_PART_TYPE_SPACER:
              /* We really should do nothing on SPACER part */
              break;
          }

        /* Some object need special recalc. */
        switch (ep->part->type)
          {
           case EDJE_PART_TYPE_TEXT:
              _edje_text_recalc_apply(ed, ep, pf, (Edje_Part_Description_Text*) chosen_desc);
              break;
           case EDJE_PART_TYPE_PROXY:
              _edje_proxy_recalc_apply(ed, ep, pf, (Edje_Part_Description_Proxy*) chosen_desc, pos);
              break;
           case EDJE_PART_TYPE_IMAGE:
              _edje_image_recalc_apply(ed, ep, pf, (Edje_Part_Description_Image*) chosen_desc, pos);
              break;
           case EDJE_PART_TYPE_BOX:
              _edje_box_recalc_apply(ed, ep, pf, (Edje_Part_Description_Box*) chosen_desc);
              break;
           case EDJE_PART_TYPE_TABLE:
              _edje_table_recalc_apply(ed, ep, pf, (Edje_Part_Description_Table*) chosen_desc);
              break;
           case EDJE_PART_TYPE_TEXTBLOCK:
              _edje_textblock_recalc_apply(ed, ep, pf, (Edje_Part_Description_Text*) chosen_desc);
              break;
           case EDJE_PART_TYPE_EXTERNAL:
           case EDJE_PART_TYPE_RECTANGLE:
           case EDJE_PART_TYPE_SWALLOW:
           case EDJE_PART_TYPE_GROUP:
              /* Nothing special to do for this type of object. */
              break;
           case EDJE_PART_TYPE_GRADIENT:
              /* FIXME: definitivly remove this code when we switch to new format. */
              abort();
              break;
           case EDJE_PART_TYPE_SPACER:
              /* We really should do nothing on SPACER part */
              break;
          }

        if (((ep->type == EDJE_RP_TYPE_SWALLOW) &&
             (ep->typedata.swallow)) &&
            (ep->typedata.swallow->swallowed_object))
          {
             //// the below really is wrong - swallow color shouldn't affect swallowed object
             //// color - the edje color as a WHOLE should though - and that should be
             //// done via the clipper anyway. this created bugs when objects had their
             //// colro set and were swallowed - then had their color changed.
             //	     evas_object_color_set(ep->typedata.swallow->swallowed_object,
             //				   (pf->color.r * pf->color.a) / 255,
             //				   (pf->color.g * pf->color.a) / 255,
             //				   (pf->color.b * pf->color.a) / 255,
             //				   pf->color.a);
             if (pf->visible)
               {
		  eo_do(ep->typedata.swallow->swallowed_object,
			evas_obj_position_set(ed->x + pf->x, ed->y + pf->y),
			evas_obj_size_set(pf->w, pf->h),
			evas_obj_visibility_set(EINA_TRUE));
               }
             else evas_object_hide(ep->typedata.swallow->swallowed_object);
             mo = ep->typedata.swallow->swallowed_object;
          }
        else mo = ep->object;
        if (chosen_desc->map.on && ep->part->type != EDJE_PART_TYPE_SPACER)
          {
             static Evas_Map *map = NULL;

             ed->have_mapped_part = EINA_TRUE;
             // create map and populate with part geometry
             if (!map) map = evas_map_new(4);
             evas_map_util_points_populate_from_object(map, ep->object);
             if (ep->part->type == EDJE_PART_TYPE_IMAGE ||
                 ((ep->part->type == EDJE_PART_TYPE_SWALLOW) &&
                  (!strcmp(evas_object_type_get(mo), "image") &&
                  (!evas_object_image_source_get(mo))))
                )
               {
                  int iw = 1, ih = 1;

                  evas_object_image_size_get(mo, &iw, &ih);
                  evas_map_point_image_uv_set(map, 0, 0.0, 0.0);
                  evas_map_point_image_uv_set(map, 1, iw , 0.0);
                  evas_map_point_image_uv_set(map, 2, iw , ih );
                  evas_map_point_image_uv_set(map, 3, 0.0, ih );
               }
             evas_map_util_3d_rotate(map,
                                     TO_DOUBLE(pf->map.rotation.x),
                                     TO_DOUBLE(pf->map.rotation.y),
                                     TO_DOUBLE(pf->map.rotation.z),
                                     pf->map.center.x, pf->map.center.y,
                                     pf->map.center.z);

             // calculate light color & position etc. if there is one
             if (pf->lighted)
               {
                  evas_map_util_3d_lighting(map,
                                            pf->map.light.x, pf->map.light.y, pf->map.light.z,
                                            pf->map.light.r, pf->map.light.g, pf->map.light.b,
                                            pf->map.light.ar, pf->map.light.ag, pf->map.light.ab);
               }

             // calculate perspective point
             if (chosen_desc->map.persp_on)
               {
                  evas_map_util_3d_perspective(map,
                                               pf->map.persp.x, pf->map.persp.y, pf->map.persp.z,
                                               pf->map.persp.focal);
               }

             // handle backface culling (object is facing away from view
             if (chosen_desc->map.backcull)
               {
                  if (pf->visible)
                    {
                       if (evas_map_util_clockwise_get(map))
                         evas_object_show(mo);
                       else evas_object_hide(mo);
                    }
               }

             // handle smooth
             if (chosen_desc->map.smooth) evas_map_smooth_set(map, 1);
             else evas_map_smooth_set(map, 0);
             // handle alpha
             if (chosen_desc->map.alpha) evas_map_alpha_set(map, 1);
             else evas_map_alpha_set(map, 0);


             if (ep->nested_smart)
               {  /* Apply map to smart obj holding nested parts */
		  eo_do(ep->nested_smart,
			evas_obj_map_set(map),
			evas_obj_map_enable_set(1));
               }
             else
               {
                  if (mo)
                    eo_do(mo,
                          evas_obj_map_set(map),
                          evas_obj_map_enable_set(1));
               }
          }
        else
          {
             if (ep->nested_smart)
               {  /* Cancel map of smart obj holding nested parts */
		  eo_do(ep->nested_smart,
			evas_obj_map_enable_set(0),
			evas_obj_map_set(NULL));
               }
             else
               {
#ifdef HAVE_EPHYSICS
                  if (!ep->body)
                    {
#endif
                       if (mo)
                         eo_do(mo,
                               evas_obj_map_enable_set(0),
                               evas_obj_map_set(NULL));
#ifdef HAVE_EPHYSICS
                    }
#endif
               }
          }
     }

#ifdef HAVE_EPHYSICS
   ep->prev_description = chosen_desc;
   if (!ep->body)
     {
#endif
   ep->x = pf->x;
   ep->y = pf->y;
   ep->w = pf->w;
   ep->h = pf->h;
#ifdef HAVE_EPHYSICS
     }
#endif

   ep->calculated |= flags;
   ep->calculating = FLAG_NONE;

#ifdef EDJE_CALC_CACHE
   if (ep->calculated == FLAG_XY)
     {
        ep->state = ed->state;
        ep->invalidate = 0;
     }
#endif
}