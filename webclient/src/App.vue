<template>
  <div id="app">
    <md-toolbar class="md-dense md-primary">
      <h3 class="md-title" style="flex: 1">Humidity Controller</h3>
      <md-button class="md-icon-button" @click="powerDialog = true">
        <md-icon>power_settings_new</md-icon>
      </md-button>
      <md-button class="md-icon-button" @click="showSettings=true">
        <md-icon>settings</md-icon>
      </md-button>
    </md-toolbar>
    <div class="md-layout md-alignment-center-center">
      <div class="md-layout-item md-size-50 md-medium-size-100">
        <Main/>
      </div>
    </div>

    <md-drawer style="max-width: 100%;" :md-active.sync="showSettings">
      <Settings v-model="showSettings"/>
    </md-drawer>

    <md-dialog :md-active.sync="powerDialog">
      <md-dialog-title>Power</md-dialog-title>
      <div class="px-20">
        <div class="md-layout md-gutter">
          <md-button class="md-raised md-accent" style="width:100%" @click="restart">Restart</md-button>
        </div>
      </div>
    <md-dialog-actions>
      <md-button class="md-primary" @click="powerDialog = false">Close</md-button>
    </md-dialog-actions>
    </md-dialog>
  </div>
</template>

<script>
import Main from './components/Main.vue'
import Settings from './components/Settings.vue'
import Service from './service'

export default {
  name: 'App',
  components: {
    Main,
    Settings
  },
  data(){
    return {
      showSettings: false,
      powerDialog: false
    }
  },
  methods:{
    restart(){
      Service.restart()
    }
  }
}
</script>

<style lang="scss">
  @import "./scss/theme.scss";
  @import "./scss/App.scss";
</style>
