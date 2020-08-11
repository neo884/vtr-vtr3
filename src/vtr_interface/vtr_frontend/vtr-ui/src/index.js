import React from "react";
import ReactDOM from "react-dom";
import io from "socket.io-client";
import "fontsource-roboto"; // for Material UI library, the font package

import { withStyles } from "@material-ui/core/styles";

import "./index.css";

import GraphMap from "./components/graph/GraphMap";
import GoalManager from "./components/goal/GoalManager";
import ToolsMenu from "./components/menu/Toolsmenu";

// SocketIO port is assumed to be UI port + 1.
// \todo For now it uses VTR2.1 socket server, change to VTR3.
const socket = io(
  window.location.hostname + ":5001" // (Number(window.location.port) + 1)
);

// Style
const styles = (theme) => ({
  vtrUI: (props) => ({
    width: "100%",
    height: "100%",
    position: "absolute",
    // backgroundColor: 'red',
    // color: props => props.color,
  }),
  graphMap: {
    position: "absolute",
    width: "100%",
    height: "100%",
    zIndex: 0,
  },
});

class VTRUI extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      // Socket IO
      socketConnected: false,
      // Tools menu
      toolsState: { pinMap: false },
      currTool: null,
      userConfirmed: false,
      // Info of adding/added goals
      addingGoalType: "Idle",
      addingGoalPath: [],
      selectedGoalPath: [],
    };
  }

  componentDidMount() {
    console.debug("[index] componentDidMount: VTRUI mounted.");

    socket.on("connect", this._handleSocketConnect.bind(this));
    socket.on("disconnect", this._handleSocketDisconnect.bind(this));
  }

  render() {
    const { classes } = this.props;
    const {
      addingGoalType,
      addingGoalPath,
      selectedGoalPath,
      toolsState,
      userConfirmed,
      socketConnected,
    } = this.state;
    return (
      <div className={classes.vtrUI}>
        <GoalManager
          socket={socket}
          socketConnected={socketConnected}
          // Select path for repeat
          addingGoalType={addingGoalType}
          setAddingGoalType={this._setAddingGoalType.bind(this)}
          addingGoalPath={addingGoalPath}
          setAddingGoalPath={this._setAddingGoalPath.bind(this)}
          setSelectedGoalPath={this._setSelectedGoalPath.bind(this)}
        ></GoalManager>
        <ToolsMenu
          toolsState={toolsState}
          selectTool={this._selectTool.bind(this)}
          requireConf={this._requireConfirmation.bind(this)}
        ></ToolsMenu>
        <GraphMap
          className={classes.graphMap}
          socket={socket}
          socketConnected={socketConnected}
          // Move graph
          pinMap={toolsState.pinMap}
          userConfirmed={userConfirmed}
          addressConf={this._addressConfirmation.bind(this)}
          // Select path for repeat
          addingGoalType={addingGoalType}
          addingGoalPath={addingGoalPath}
          setAddingGoalPath={this._setAddingGoalPath.bind(this)}
          selectedGoalPath={selectedGoalPath}
        />
      </div>
    );
  }

  /** Socket IO callbacks */
  _handleSocketConnect() {
    this.setState({ socketConnected: true });
    console.log("Socket IO connected.");
  }
  _handleSocketDisconnect() {
    this.setState({ socketConnected: false });
    console.log("Socket disconnected.");
  }

  /** Tools menu callbacks */
  _selectTool(tool) {
    this.setState((state) => {
      // User selects a tool
      if (!state.currTool) {
        console.log("[index] _selectTool: User selects ", tool);
        return {
          toolsState: {
            ...state.toolsState,
            [tool]: true,
          },
          currTool: tool,
        };
      }
      // User de-selects a tool
      if (state.currTool === tool) {
        console.log("[index] _selectTool: User de-selects ", tool);
        return {
          toolsState: {
            ...state.toolsState,
            [tool]: false,
          },
          currTool: null,
        };
      }
      // User selects a different tool without de-selecting the previous one, so
      // quit the previous one.
      console.log("[index] _selectTool: User switches to ", tool);
      return {
        toolsState: {
          ...state.toolsState,
          [state.currTool]: false,
          [tool]: true,
        },
        currTool: tool,
      };
    });
  }
  _requireConfirmation() {
    this.setState((state) => {
      Object.keys(state.toolsState).forEach(function (key) {
        state.toolsState[key] = false;
      });
      return {
        toolsState: state.toolsState,
        currTool: null,
        userConfirmed: true,
      };
    });
  }
  _addressConfirmation() {
    console.log("[index] _addressConfirmation: Confirmation addressed.");
    this.setState({ userConfirmed: false });
  }

  /** Sets type of the current goal to be added. */
  _setAddingGoalType(type) {
    this.setState({ addingGoalType: type });
  }

  /** Sets target vertices of the current goal to be added. */
  _setAddingGoalPath(path) {
    this.setState({ addingGoalPath: path });
  }

  /** Sets target vertices of the current selected goal.
   *
   * @param {array} path Path of the selected goal.
   */
  _setSelectedGoalPath(path) {
    this.setState({ selectedGoalPath: path });
  }
}

var VTRUIStyled = withStyles(styles)(VTRUI);

// ========================================

ReactDOM.render(<VTRUIStyled />, document.getElementById("root"));
