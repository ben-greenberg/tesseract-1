import QtQuick 2.4

JointManipulationWidgetForm {
    function create()
    {

    }

    jointSpinBox.onValueChanged: { jointSlider.value = jointSpinBox.value }
    jointSlider.onValueChanged: { jointSpinBox.value = jointSlider.value }

}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
