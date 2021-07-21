#Usage perl -n -i.u1 update_computed_fields.160508.pl computed_field_component_operations.cpp

BEGIN
  {
	 $print = 1;
	 $first_date = 1;
  }

if ($first_date)
  {
	 if (s/LAST MODIFIED : .*/LAST MODIFIED : 16 May 2008/)
	 {
		undef $first_date;
	 }
  }


s/Computed_field_set_type_(\w+)/Computed_field_create_${1}/;
if (m/(int |Computed_field \*)Computed_field_create_\w+\(/)
  {
	 s/int Computed_field_create_(\w+)\(struct Computed_field \*field,/Computed_field *Computed_field_create_${1}(/;
	 $in_create_function = 1;
  }
if ($in_create_function)
  {
	 s/LAST MODIFIED : .*/LAST MODIFIED : 16 May 2008/;
	 s/^(\s*)struct Computed_field ([\w\*,]+);/${1}Computed_field *field, $2;/;
	 s/if \(field\s*&&/if \(/;
	 s/return_code,//;
	 s/,\s?return_code//;
	 if (m/return_code\s*=\s*1;/)
		{
		  $print = 2;
		}
	 s/return_code\s*=\s*0;/field = (Computed_field *)NULL;/;
	 s/return\s*\(return_code\);/return (field);/;
	 s#/\* 2\. free current type\-specific data \*/#/* 2. create new field */#;
	 s#Computed_field_clear_type\(field\);#field = ACCESS(Computed_field)(CREATE(Computed_field)(""));#;

	 if (m%/\* Computed_field_create_\w+ \*/%)
		{
		  undef $in_create_function;
		}
  }

if (s/Computed_field_create_(\w+)\(field,/Computed_field_copy_type_specific_and_deaccess(field, Computed_field_create_$1(/)
  {
	 $need_extra_closing_bracket = 1;
  }
if ($need_extra_closing_bracket and s/\);/));/)
  {
	 undef $need_extra_closing_bracket;
  }



if (defined $print_when_match)
  {
	 if (m#$print_when_match#)
		{
		  $print = 1;
		  undef $print_when_match;
		}
  }
if ($print == 1)
  {
	 print;
  }
if ($print > 1)
  {
	 $print--;
  }
