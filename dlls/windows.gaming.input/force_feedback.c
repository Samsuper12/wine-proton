/* WinRT Windows.Gaming.Input implementation
 *
 * Copyright 2022 Rémi Bernon for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "private.h"

#include "ddk/hidsdi.h"
#include "dinput.h"
#include "hidusage.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(input);

struct motor
{
    IForceFeedbackMotor IForceFeedbackMotor_iface;
    LONG ref;

    IDirectInputDevice8W *device;
};

static inline struct motor *impl_from_IForceFeedbackMotor( IForceFeedbackMotor *iface )
{
    return CONTAINING_RECORD( iface, struct motor, IForceFeedbackMotor_iface );
}

static HRESULT WINAPI motor_QueryInterface( IForceFeedbackMotor *iface, REFIID iid, void **out )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );

    TRACE( "iface %p, iid %s, out %p.\n", iface, debugstr_guid( iid ), out );

    if (IsEqualGUID( iid, &IID_IUnknown ) ||
        IsEqualGUID( iid, &IID_IInspectable ) ||
        IsEqualGUID( iid, &IID_IAgileObject ) ||
        IsEqualGUID( iid, &IID_IForceFeedbackMotor ))
    {
        IInspectable_AddRef( (*out = &impl->IForceFeedbackMotor_iface) );
        return S_OK;
    }

    FIXME( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid( iid ) );
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI motor_AddRef( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    ULONG ref = InterlockedIncrement( &impl->ref );
    TRACE( "iface %p increasing refcount to %lu.\n", iface, ref );
    return ref;
}

static ULONG WINAPI motor_Release( IForceFeedbackMotor *iface )
{
    struct motor *impl = impl_from_IForceFeedbackMotor( iface );
    ULONG ref = InterlockedDecrement( &impl->ref );

    TRACE( "iface %p decreasing refcount to %lu.\n", iface, ref );

    if (!ref)
    {
        IDirectInputDevice8_Release( impl->device );
        free( impl );
    }

    return ref;
}

static HRESULT WINAPI motor_GetIids( IForceFeedbackMotor *iface, ULONG *iid_count, IID **iids )
{
    FIXME( "iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_GetRuntimeClassName( IForceFeedbackMotor *iface, HSTRING *class_name )
{
    return WindowsCreateString( RuntimeClass_Windows_Gaming_Input_ForceFeedback_ForceFeedbackMotor,
                                ARRAY_SIZE(RuntimeClass_Windows_Gaming_Input_ForceFeedback_ForceFeedbackMotor),
                                class_name );
}

static HRESULT WINAPI motor_GetTrustLevel( IForceFeedbackMotor *iface, TrustLevel *trust_level )
{
    FIXME( "iface %p, trust_level %p stub!\n", iface, trust_level );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_get_AreEffectsPaused( IForceFeedbackMotor *iface, BOOLEAN *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_get_MasterGain( IForceFeedbackMotor *iface, double *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_put_MasterGain( IForceFeedbackMotor *iface, double value )
{
    FIXME( "iface %p, value %#I64x stub!\n", iface, *(UINT64 *)&value );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_get_IsEnabled( IForceFeedbackMotor *iface, BOOLEAN *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_get_SupportedAxes( IForceFeedbackMotor *iface, enum ForceFeedbackEffectAxes *value )
{
    FIXME( "iface %p, value %p stub!\n", iface, value );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_LoadEffectAsync( IForceFeedbackMotor *iface, IForceFeedbackEffect *effect,
                                             IAsyncOperation_ForceFeedbackLoadEffectResult **async_op )
{
    FIXME( "iface %p, effect %p, async_op %p stub!\n", iface, effect, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_PauseAllEffects( IForceFeedbackMotor *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_ResumeAllEffects( IForceFeedbackMotor *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_StopAllEffects( IForceFeedbackMotor *iface )
{
    FIXME( "iface %p stub!\n", iface );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_TryDisableAsync( IForceFeedbackMotor *iface, IAsyncOperation_boolean **async_op )
{
    FIXME( "iface %p, async_op %p stub!\n", iface, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_TryEnableAsync( IForceFeedbackMotor *iface, IAsyncOperation_boolean **async_op )
{
    FIXME( "iface %p, async_op %p stub!\n", iface, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_TryResetAsync( IForceFeedbackMotor *iface, IAsyncOperation_boolean **async_op )
{
    FIXME( "iface %p, async_op %p stub!\n", iface, async_op );
    return E_NOTIMPL;
}

static HRESULT WINAPI motor_TryUnloadEffectAsync( IForceFeedbackMotor *iface, IForceFeedbackEffect *effect,
                                                  IAsyncOperation_boolean **async_op )
{
    FIXME( "iface %p, effect %p, async_op %p stub!\n", iface, effect, async_op );
    return E_NOTIMPL;
}

static const struct IForceFeedbackMotorVtbl motor_vtbl =
{
    motor_QueryInterface,
    motor_AddRef,
    motor_Release,
    /* IInspectable methods */
    motor_GetIids,
    motor_GetRuntimeClassName,
    motor_GetTrustLevel,
    /* IForceFeedbackMotor methods */
    motor_get_AreEffectsPaused,
    motor_get_MasterGain,
    motor_put_MasterGain,
    motor_get_IsEnabled,
    motor_get_SupportedAxes,
    motor_LoadEffectAsync,
    motor_PauseAllEffects,
    motor_ResumeAllEffects,
    motor_StopAllEffects,
    motor_TryDisableAsync,
    motor_TryEnableAsync,
    motor_TryResetAsync,
    motor_TryUnloadEffectAsync,
};

HRESULT force_feedback_motor_create( IDirectInputDevice8W *device, IForceFeedbackMotor **out )
{
    struct motor *impl;

    TRACE( "device %p, out %p\n", device, out );

    if (!(impl = calloc( 1, sizeof(*impl) ))) return E_OUTOFMEMORY;
    impl->IForceFeedbackMotor_iface.lpVtbl = &motor_vtbl;
    impl->ref = 1;

    IDirectInputDevice_AddRef( device );
    impl->device = device;

    *out = &impl->IForceFeedbackMotor_iface;
    TRACE( "created ForceFeedbackMotor %p\n", *out );
    return S_OK;
}