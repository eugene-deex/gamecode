#include "AmmoWidget.h"

void UAmmoWidget::UpdateAmmoCounter(int32 NewAmmo, int32 NewTotalAmmo)
{
	Ammo = NewAmmo;
	TotalAmmo = NewTotalAmmo;
}

void UAmmoWidget::UpdateGrenadesAmmoCounter(int32 NewGrenades, int32 MaxAmmo)
{
	Grenades = NewGrenades;
	MaxGrenades = MaxAmmo;
}
